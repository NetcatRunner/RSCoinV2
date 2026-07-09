#include "protocol/rscoin/RSCoinProtocol.hpp"

#include <utility>
#include <vector>

#include "log/Logger.hpp"
#include "primitives/Codec.hpp"
#include "protocol/Message.hpp"

namespace RSCoin::Protocol {

    RSCoinProtocol::RSCoinProtocol(Network::INetwork& network, Settings settings, NodeServices services)
        : _network(network), _services(services), _version(settings.version), _chainId(settings.chainId) {
        registerHandlers();

        _services.manager.subscribe([this](const Primitives::Block& block, Chain::ImportOrigin) { announceBlock(block); });
        _services.mempool.subscribe([this](const Primitives::Transaction& transaction) { announceTransaction(transaction); });
    }

    void RSCoinProtocol::registerHandlers() {
        _dispatcher.on<StatusMessage>([this](const auto& from, const auto& m) { handleStatus(from, m); });
        _dispatcher.on<PingMessage>([this](const auto& from, const auto& m) { handlePing(from, m); });
        _dispatcher.on<PongMessage>([this](const auto& from, const auto& m) { handlePong(from, m); });
        _dispatcher.on<NewBlockMessage>([this](const auto& from, const auto& m) { handleNewBlock(from, m); });
        _dispatcher.on<GetBlocksMessage>([this](const auto& from, const auto& m) { handleGetBlocks(from, m); });
        _dispatcher.on<BlocksMessage>([this](const auto& from, const auto& m) { handleBlocks(from, m); });
        _dispatcher.on<NewTransactionMessage>([this](const auto& from, const auto& m) { handleNewTransaction(from, m); });
    }

    void RSCoinProtocol::onPeerConnected(const Network::PeerId& peer) {
        {
            std::lock_guard lock(_mutex);
            _sessions[peer] = PeerSession{};
        }
        (void)Protocol::send(_network, peer, StatusMessage{.version = _version,
                                                            .chainId = _chainId,
                                                            .height = _services.chain.height(),
                                                            .headHash = _services.chain.headHash()});
    }

    void RSCoinProtocol::onPeerDisconnected(const Network::PeerId& peer) {
        std::lock_guard lock(_mutex);
        _sessions.erase(peer);
    }

    void RSCoinProtocol::onMessage(const Network::PeerId& from, Network::NetMessage message) {
        const auto handled = _dispatcher.dispatch(from, message);
        if (!handled) {
            dropPeer(from, handled.error());
            return;
        }
        if (!*handled)
            RSCoin_DEBUG("unhandled topic {} from {}", message.topic, from.value);
    }

    void RSCoinProtocol::tick() {
        const auto now = std::chrono::steady_clock::now();

        std::vector<std::pair<Network::PeerId, PingMessage>> toPing;
        {
            std::lock_guard lock(_mutex);
            for (auto& [peer, session] : _sessions) {
                if (!session.ready || session.pingOutstanding || now - session.lastPing < kPingInterval)
                    continue;
                session.pingOutstanding = true;
                session.pingNonce = _nextPingNonce++;
                session.lastPing = now;
                toPing.emplace_back(peer, PingMessage{.nonce = session.pingNonce});
            }
        }
        for (const auto& [peer, ping] : toPing)
            (void)Protocol::send(_network, peer, ping);
    }

    void RSCoinProtocol::handleStatus(const Network::PeerId& from, const StatusMessage& status) {
        if (status.version != _version || status.chainId != _chainId) {
            dropPeer(from, core::Error{core::ErrorCode::protocol, 
                "incompatible peer (version " + std::to_string(status.version) + ", chainId " + std::to_string(status.chainId) + ")"});
            return;
        }

        bool behind = false;
        {
            std::lock_guard lock(_mutex);
            auto it = _sessions.find(from);
            if (it == _sessions.end())
                return;
            it->second.ready = true;
            it->second.peerHeight = status.height;
            behind = status.height > _services.chain.height();
        }
        RSCoin_INFO("handshake completed with {} (chainId {}, height {})", from.value, status.chainId, status.height);

        if (behind)
            requestBlocks(from, _services.chain.height() + 1);
    }

    void RSCoinProtocol::handlePing(const Network::PeerId& from, const PingMessage& ping) {
        (void)Protocol::send(_network, from, PongMessage{.nonce = ping.nonce});
    }

    void RSCoinProtocol::handlePong(const Network::PeerId& from, const PongMessage& pong) {
        std::lock_guard lock(_mutex);
        auto it = _sessions.find(from);
        if (it != _sessions.end() && it->second.pingOutstanding && it->second.pingNonce == pong.nonce)
            it->second.pingOutstanding = false;
    }

    void RSCoinProtocol::handleNewBlock(const Network::PeerId& from, const NewBlockMessage& message) {
        const core::Hash256 hash = _services.hasher.hash(Primitives::encode(message.block.header));
        {
            std::lock_guard lock(_mutex);
            auto it = _sessions.find(from);
            if (it == _sessions.end() || !it->second.ready)
                return;
            if (it->second.seenBlocks.size() >= kSeenBlocksLimit)
                it->second.seenBlocks.clear();
            it->second.seenBlocks.insert(hash);
            if (message.block.header.height > it->second.peerHeight)
                it->second.peerHeight = message.block.header.height;
        }
        RSCoin_INFO("received block {} from {}", message.block.header.height, from.value);
        importFrom(from, message.block);
    }

    void RSCoinProtocol::handleGetBlocks(const Network::PeerId& from, const GetBlocksMessage& request) {
        BlocksMessage reply;
        const std::uint32_t count = std::min(request.maxCount, kMaxBlocksPerRequest);
        reply.blocks.reserve(count);

        for (std::uint32_t i = 0; i < count; ++i) {
            auto block = _services.chain.blockByHeight(request.fromHeight + i);
            if (!block)
                break;
            reply.blocks.push_back(std::move(*block));
        }
        RSCoin_INFO("serving {} block(s) from height {} to {}", reply.blocks.size(), request.fromHeight, from.value);
        (void)Protocol::send(_network, from, reply);
    }

    void RSCoinProtocol::handleBlocks(const Network::PeerId& from, const BlocksMessage& message) {
        std::size_t imported = 0;
        for (const auto& block : message.blocks) {
            auto outcome = _services.manager.importBlock(block, Chain::ImportOrigin::remote);
            if (!outcome) {
                RSCoin_WARN("sync import failed: {}", outcome.error().describe());
                return;
            }
            if (*outcome == Chain::ImportOutcome::invalid) {
                dropPeer(from, core::Error{core::ErrorCode::protocol, "peer sent an invalid block"});
                return;
            }
            if (*outcome == Chain::ImportOutcome::imported)
                ++imported;
        }
        if (!message.blocks.empty())
            RSCoin_INFO("sync: received {} block(s) from {}, imported {} (local height {})", message.blocks.size(), from.value, imported, _services.chain.height());

        std::uint64_t peerHeight = 0;
        {
            std::lock_guard lock(_mutex);
            if (auto it = _sessions.find(from); it != _sessions.end())
                peerHeight = it->second.peerHeight;
        }
        if (!message.blocks.empty() && peerHeight > _services.chain.height())
            requestBlocks(from, _services.chain.height() + 1);
    }

    void RSCoinProtocol::handleNewTransaction(const Network::PeerId& from, const NewTransactionMessage& message) {
        const core::Hash256 hash = _services.hasher.hash(Primitives::encode(message.transaction));
        {
            std::lock_guard lock(_mutex);
            auto it = _sessions.find(from);
            if (it == _sessions.end() || !it->second.ready)
                return;
            if (it->second.seenTxs.size() >= kSeenTxsLimit)
                it->second.seenTxs.clear();
            it->second.seenTxs.insert(hash);
        }

        const auto outcome = _services.mempool.add(message.transaction);
        if (!outcome)
            RSCoin_WARN("mempool admission failed: {}", outcome.error().describe());
    }

    void RSCoinProtocol::importFrom(const Network::PeerId& from, const Primitives::Block& block) {
        auto outcome = _services.manager.importBlock(block, Chain::ImportOrigin::remote);
        if (!outcome) {
            RSCoin_WARN("import failed: {}", outcome.error().describe());
            return;
        }
        switch (*outcome) {
            case Chain::ImportOutcome::imported:
                RSCoin_INFO("imported block {} from {} (new head)", block.header.height, from.value);
                break;
            case Chain::ImportOutcome::orphaned:
                RSCoin_INFO("block {} from {} is orphaned — requesting catch-up from height {}", block.header.height, from.value, _services.chain.height() + 1);
                requestBlocks(from, _services.chain.height() + 1);
                break;
            case Chain::ImportOutcome::invalid:
                dropPeer(from, core::Error{core::ErrorCode::protocol, "peer sent an invalid block"});
                break;
            default:
                break;
        }
    }

    void RSCoinProtocol::announceBlock(const Primitives::Block& block) {
        const core::Hash256 hash = _services.hasher.hash(Primitives::encode(block.header));

        std::vector<Network::PeerId> targets;
        {
            std::lock_guard lock(_mutex);
            for (auto& [peer, session] : _sessions) {
                if (!session.ready || session.seenBlocks.contains(hash))
                    continue;
                if (session.seenBlocks.size() >= kSeenBlocksLimit)
                    session.seenBlocks.clear();
                session.seenBlocks.insert(hash);
                targets.push_back(peer);
            }
        }

        if (targets.empty())
            return;
        RSCoin_INFO("announcing block {} to {} peer(s)", block.header.height, targets.size());
        const NewBlockMessage message{block};
        for (const auto& peer : targets) {
            (void)Protocol::send(_network, peer, message);
        }
    }

    void RSCoinProtocol::announceTransaction(const Primitives::Transaction& transaction) {
        const core::Hash256 hash = _services.hasher.hash(Primitives::encode(transaction));

        std::vector<Network::PeerId> targets;
        {
            std::lock_guard lock(_mutex);
            for (auto& [peer, session] : _sessions) {
                if (!session.ready || session.seenTxs.contains(hash))
                    continue;
                if (session.seenTxs.size() >= kSeenTxsLimit)
                    session.seenTxs.clear();
                session.seenTxs.insert(hash);
                targets.push_back(peer);
            }
        }

        if (targets.empty())
            return;
        const NewTransactionMessage message{transaction};
        for (const auto& peer : targets)
            (void)Protocol::send(_network, peer, message);
    }

    void RSCoinProtocol::requestBlocks(const Network::PeerId& peer, std::uint64_t fromHeight) {
        RSCoin_INFO("requesting blocks from height {} ({})", fromHeight, peer.value);
        (void)Protocol::send(_network, peer, GetBlocksMessage{.fromHeight = fromHeight, .maxCount = kMaxBlocksPerRequest});
    }

    void RSCoinProtocol::dropPeer(const Network::PeerId& peer, const core::Error& reason) {
        RSCoin_WARN("dropping peer {}: {}", peer.value, reason.describe());
        _network.disconnect(peer);
    }

}

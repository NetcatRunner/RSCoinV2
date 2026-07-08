#include "network/TcpNetwork.hpp"

#include <array>
#include <charconv>
#include <utility>

#include "log/Logger.hpp"
#include "network/Wire.hpp"

namespace RSCoin::Network {

    namespace {
        core::Result<Endpoint> parseEndpoint(const std::string& address) {
            const auto colon = address.rfind(':');
            if (colon == std::string::npos || colon + 1 >= address.size())
                return core::fail(core::ErrorCode::config, "invalid endpoint '" + address + "' (expected host:port)");

            Endpoint endpoint;
            endpoint.host = address.substr(0, colon);

            const char* first = address.data() + colon + 1;
            const char* last = address.data() + address.size();
            if (std::from_chars(first, last, endpoint.port).ec != std::errc{})
                return core::fail(core::ErrorCode::config, "invalid port in endpoint '" + address + "'");
            return endpoint;
        }
    }

    TcpNetwork::TcpNetwork(Config::NetworkConfig config) : _config(std::move(config)) {}

    TcpNetwork::~TcpNetwork() { stop(); }

    core::Result<void> TcpNetwork::start(INetworkObserver& observer) {
        if (_running.exchange(true))
            return core::fail(core::ErrorCode::network, "transport already started");
        _observer = &observer;

        auto listener = Socket::listen({_config.listenAddress, _config.port});
        if (!listener) {
            _running = false;
            return core::fail(listener.error(), "starting transport");
        }
        _listener = std::move(*listener);
        _acceptThread = std::thread([this] { acceptLoop(); });
        RSCoin_INFO("listening on {}:{}", _config.listenAddress, _config.port);

        for (const auto& address : _config.bootstrapPeers) {
            auto endpoint = parseEndpoint(address);
            if (!endpoint) {
                RSCoin_WARN("bootstrap peer skipped: {}", endpoint.error().message);
                continue;
            }
            auto connected = connect(*endpoint);
            if (!connected)
                RSCoin_WARN("bootstrap peer unreachable: {}", connected.error().message);
        }
        return {};
    }

    void TcpNetwork::stop() {
        if (!_running.exchange(false))
            return;

        _listener.shutdown();
        if (_acceptThread.joinable())
            _acceptThread.join();

        std::vector<std::thread> readers;
        {
            std::lock_guard lock(_mutex);
            for (auto& [id, peer] : _peers)
                peer->socket.shutdown();
            readers = std::move(_readers);
        }
        for (auto& reader : readers) {
            if (reader.joinable()) {
                reader.join();
            }
        }

        std::lock_guard lock(_mutex);
        _peers.clear();
    }

    core::Result<void> TcpNetwork::connect(const Endpoint& endpoint) {
        auto socket = Socket::connect(endpoint);
        if (!socket)
            return core::fail(socket.error());
        return addPeer(std::move(*socket));
    }

    void TcpNetwork::disconnect(const PeerId& peer) {
        if (auto found = findPeer(peer))
            found->socket.shutdown();
    }

    core::Result<void> TcpNetwork::broadcast(NetMessage message) {
        const core::Bytes frame = Wire::encode(message);

        std::vector<std::shared_ptr<Peer>> peers;
        {
            std::lock_guard lock(_mutex);
            peers.reserve(_peers.size());
            for (const auto& [id, peer] : _peers)
                peers.push_back(peer);
        }
        for (const auto& peer : peers) {
            std::lock_guard lock(peer->writeMutex);
            (void)peer->socket.sendAll(frame);
        }
        return {};
    }

    core::Result<void> TcpNetwork::send(const PeerId& peer, NetMessage message) {
        auto found = findPeer(peer);
        if (!found)
            return core::fail(core::ErrorCode::notFound, "unknown peer: " + peer.value);

        const core::Bytes frame = Wire::encode(message);
        std::lock_guard lock(found->writeMutex);
        return found->socket.sendAll(frame);
    }

    std::size_t TcpNetwork::peerCount() const {
        std::lock_guard lock(_mutex);
        return _peers.size();
    }

    void TcpNetwork::acceptLoop() {
        while (_running) {
            auto accepted = _listener.accept();
            if (!accepted)
                break;

            auto added = addPeer(std::move(*accepted));
            if (!added)
                RSCoin_WARN("inbound connection rejected: {}", added.error().message);
        }
    }

    void TcpNetwork::readerLoop(std::shared_ptr<Peer> peer) {
        while (_running) {
            std::array<std::byte, Wire::kHeaderSize> rawHeader{};
            if (!peer->socket.receiveExact(rawHeader))
                break;

            const Wire::Header header = Wire::decodeHeader(rawHeader);
            if (header.length > _config.maxMessageBytes) {
                RSCoin_WARN("oversized frame from {} ({} bytes) — disconnecting", peer->id.value, header.length);
                break;
            }

            core::Bytes payload(header.length);
            if (header.length > 0 && !peer->socket.receiveExact(payload))
                break;

            _observer->onMessage(peer->id, NetMessage{header.topic, std::move(payload)});
        }
        removePeer(peer->id);
    }

    core::Result<void> TcpNetwork::addPeer(Socket socket) {
        auto peer = std::make_shared<Peer>();
        {
            std::lock_guard lock(_mutex);
            if (_peers.size() >= _config.maxPeers)
                return core::fail(core::ErrorCode::network, "peer limit reached (" + std::to_string(_config.maxPeers) + ")");

            peer->id = PeerId{socket.remoteAddress() + "#" + std::to_string(_nextPeerNumber++)};
            peer->socket = std::move(socket);
            _peers[peer->id] = peer;
            _readers.emplace_back([this, peer] { readerLoop(peer); });
        }

        RSCoin_INFO("peer connected: {}", peer->id.value);
        _observer->onPeerConnected(peer->id);
        return {};
    }

    void TcpNetwork::removePeer(const PeerId& id) {
        {
            std::lock_guard lock(_mutex);
            if (_peers.erase(id) == 0)
                return;
        }
        if (_running) {
            RSCoin_INFO("peer disconnected: {}", id.value);
            _observer->onPeerDisconnected(id);
        }
    }

    std::shared_ptr<Peer> TcpNetwork::findPeer(const PeerId& id) const {
        std::lock_guard lock(_mutex);
        auto it = _peers.find(id);
        return it == _peers.end() ? nullptr : it->second;
    }

}

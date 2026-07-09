#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>


#include "protocol/Dispatcher.hpp"
#include "protocol/NodeServices.hpp"
#include "protocol/IProtocol.hpp"
#include "protocol/rscoin/Messages.hpp"

namespace RSCoin::Protocol {

    class RSCoinProtocol : public IProtocol {
    public:
        struct Settings {
            std::uint32_t version{};
            std::uint64_t chainId{};
        };

        RSCoinProtocol(Network::INetwork& network, Settings settings, NodeServices services);

        std::string_view name() const noexcept override { return "rscoin"; }
        void tick() override;

        void onPeerConnected(const Network::PeerId& peer) override;
        void onPeerDisconnected(const Network::PeerId& peer) override;
        void onMessage(const Network::PeerId& from, Network::NetMessage message) override;

    private:
        struct PeerSession {
            bool ready{false};
            std::uint64_t peerHeight{};
            std::set<core::Hash256> seenBlocks;
            std::set<core::Hash256> seenTxs;
            bool pingOutstanding{false};
            std::uint64_t pingNonce{};
            std::chrono::steady_clock::time_point lastPing{};
        };

        static constexpr std::chrono::seconds kPingInterval{30};
        static constexpr std::uint32_t kMaxBlocksPerRequest = 64;
        static constexpr std::size_t kSeenBlocksLimit = 4096;
        static constexpr std::size_t kSeenTxsLimit = 4096;

        void registerHandlers();
        void handleStatus(const Network::PeerId& from, const StatusMessage& status);
        void handlePing(const Network::PeerId& from, const PingMessage& ping);
        void handlePong(const Network::PeerId& from, const PongMessage& pong);
        void handleNewBlock(const Network::PeerId& from, const NewBlockMessage& message);
        void handleGetBlocks(const Network::PeerId& from, const GetBlocksMessage& request);
        void handleBlocks(const Network::PeerId& from, const BlocksMessage& message);
        void handleNewTransaction(const Network::PeerId& from, const NewTransactionMessage& message);

        void announceBlock(const Primitives::Block& block);
        void announceTransaction(const Primitives::Transaction& transaction);
        void requestBlocks(const Network::PeerId& peer, std::uint64_t fromHeight);
        void importFrom(const Network::PeerId& from, const Primitives::Block& block);
        void dropPeer(const Network::PeerId& peer, const core::Error& reason);

        Network::INetwork& _network;
        NodeServices _services;
        std::uint32_t _version;
        std::uint64_t _chainId;
        Dispatcher _dispatcher;

        std::mutex _mutex;
        std::map<Network::PeerId, PeerSession> _sessions;
        std::uint64_t _nextPingNonce{1};
    };

}

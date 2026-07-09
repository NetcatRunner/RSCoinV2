#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <functional>

#include "network/NetworkConfig.hpp"
#include "network/INetwork.hpp"
#include "network/tcp/Peer.hpp"
#include "network/tcp/Socket.hpp"

namespace std {
    template <>
    struct hash<RSCoin::Network::PeerId> {
        std::size_t operator()(const RSCoin::Network::PeerId& peer) const noexcept {
            return std::hash<std::string>{}(peer.value);
        }
    };
}

namespace RSCoin::Network {


    class TcpNetwork : public INetwork {
    public:
        TcpNetwork(NetworkConfig config);
        ~TcpNetwork() override;

        core::Result<void> start(INetworkObserver& observer) override;
        void stop() override;

        core::Result<void> connect(const Endpoint& endpoint) override;
        void disconnect(const PeerId& peer) override;

        core::Result<void> broadcast(NetMessage message) override;
        core::Result<void> send(const PeerId& peer, NetMessage message) override;

        std::size_t peerCount() const override;

    private:
        void acceptLoop();
        void readerLoop(std::shared_ptr<Peer> peer);

        core::Result<void> addPeer(Socket socket);
        void removePeer(const PeerId& id);
        std::shared_ptr<Peer> findPeer(const PeerId& id) const;

        NetworkConfig _config;
        INetworkObserver* _observer = nullptr;
        std::atomic<bool> _running{false};

        Socket _listener;
        std::thread _acceptThread;

        mutable std::mutex _mutex;
        std::unordered_map<PeerId, std::shared_ptr<Peer>> _peers;
        std::vector<std::thread> _readers;
        std::uint64_t _nextPeerNumber{};
    };
}

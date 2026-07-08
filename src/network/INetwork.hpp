#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Network {

    struct PeerId {
        std::string value;
        auto operator<=>(const PeerId&) const = default;
    };

    struct Endpoint {
        std::string host;
        std::uint16_t port{};
    };

    struct NetMessage {
        std::uint16_t topic{};
        core::Bytes payload;
    };

    class INetworkObserver {
    public:
        virtual ~INetworkObserver() = default;

        virtual void onPeerConnected(const PeerId& peer) = 0;
        virtual void onPeerDisconnected(const PeerId& peer) = 0;
        virtual void onMessage(const PeerId& from, NetMessage message) = 0;
    };

    class INetwork {
    public:
        virtual ~INetwork() = default;

        virtual core::Result<void> start(INetworkObserver& observer) = 0;
        virtual void stop() = 0;

        virtual core::Result<void> connect(const Endpoint& endpoint) = 0;
        virtual void disconnect(const PeerId& peer) = 0;

        virtual core::Result<void> broadcast(NetMessage message) = 0;
        virtual core::Result<void> send(const PeerId& peer, NetMessage message) = 0;

        virtual std::size_t peerCount() const = 0;
    };

}

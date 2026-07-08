#pragma once

#include <concepts>
#include <cstdint>

#include "core/Result.hpp"
#include "network/INetwork.hpp"

namespace RSCoin::Protocol {

    template <typename T>
    concept Message = requires(const T& message, core::BytesView payload) {
        { T::kTopic } -> std::convertible_to<std::uint16_t>;
        { message.encode() } -> std::same_as<core::Bytes>;
        { T::decode(payload) } -> std::same_as<core::Result<T>>;
    };

    template <Message T>
    core::Result<void> send(Network::INetwork& network, const Network::PeerId& peer, const T& message) {
        return network.send(peer, Network::NetMessage{T::kTopic, message.encode()});
    }

    template <Message T>
    core::Result<void> broadcast(Network::INetwork& network, const T& message) {
        return network.broadcast(Network::NetMessage{T::kTopic, message.encode()});
    }

}

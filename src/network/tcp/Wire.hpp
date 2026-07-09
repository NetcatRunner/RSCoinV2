#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/Types.hpp"
#include "network/INetwork.hpp"

namespace RSCoin::Network {

    class Wire {
    public:
        static constexpr std::size_t kHeaderSize = 6;

        struct Header {
            std::uint16_t topic{};
            std::uint32_t length{};
        };

        static core::Bytes encode(const NetMessage& message);
        static Header decodeHeader(std::span<const std::byte, kHeaderSize> raw);
    };

}

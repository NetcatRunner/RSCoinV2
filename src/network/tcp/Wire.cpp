#include "network/Wire.hpp"

namespace RSCoin::Network {

    core::Bytes Wire::encode(const NetMessage& message) {
        core::Bytes frame;
        frame.reserve(kHeaderSize + message.payload.size());

        const auto length = static_cast<std::uint32_t>(message.payload.size());
        frame.push_back(static_cast<std::byte>(message.topic & 0xFF));
        frame.push_back(static_cast<std::byte>((message.topic >> 8) & 0xFF));
        for (unsigned shift = 0; shift < 32; shift += 8)
            frame.push_back(static_cast<std::byte>((length >> shift) & 0xFF));

        frame.insert(frame.end(), message.payload.begin(), message.payload.end());
        return frame;
    }

    Wire::Header Wire::decodeHeader(std::span<const std::byte, kHeaderSize> raw) {
        Header header;
        header.topic = static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(raw[0]) | (std::to_integer<std::uint16_t>(raw[1]) << 8));
        for (unsigned i = 0; i < 4; ++i)
            header.length |= std::to_integer<std::uint32_t>(raw[2 + i]) << (8 * i);
        return header;
    }

}

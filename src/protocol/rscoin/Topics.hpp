#pragma once

#include <cstdint>

namespace RSCoin::Protocol {

    namespace Topics {
        // 0x00xx : session
        inline constexpr std::uint16_t kStatus = 0x0000;
        inline constexpr std::uint16_t kPing = 0x0001;
        inline constexpr std::uint16_t kPong = 0x0002;

        // 0x01xx : chaîne
        inline constexpr std::uint16_t kNewBlock = 0x0100;
        inline constexpr std::uint16_t kGetBlocks = 0x0101;
        inline constexpr std::uint16_t kBlocks = 0x0102;

        // 0x02xx : transactions
        inline constexpr std::uint16_t kNewTransaction = 0x0200;
    }

}

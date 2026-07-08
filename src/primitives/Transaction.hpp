#pragma once

#include <cstdint>

#include "core/Types.hpp"

namespace RSCoin::Primitives {

    struct Transaction {
        static constexpr std::uint8_t kLegacyType = 0;

        std::uint8_t type{kLegacyType};
        core::Address from;
        core::Address to;
        core::Amount value;
        std::uint64_t nonce{};
        core::Bytes payload;
        core::Signature signature;
    };

}

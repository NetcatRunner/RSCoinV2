#pragma once

#include <cstdint>
#include <vector>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Chain {

    struct BlockConfig {
        static constexpr const char* kSection = "block";

        std::size_t maxTransactions{};
        std::size_t maxExtraDataBytes{};
        std::vector<std::uint16_t> enabledExtensions;

        static core::Result<BlockConfig> from(const Config::Section& section);
    };

}

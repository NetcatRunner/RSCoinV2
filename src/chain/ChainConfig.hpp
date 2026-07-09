#pragma once

#include <cstdint>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Chain {

    struct ChainConfig {
        static constexpr const char* kSection = "chain";

        std::uint64_t id{};

        static core::Result<ChainConfig> from(const Config::Section& section);
    };

}

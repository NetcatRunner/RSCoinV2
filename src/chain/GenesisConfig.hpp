#pragma once

#include <cstdint>
#include <vector>

#include "config/Section.hpp"
#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Chain {

    struct GenesisAccount {
        core::Address address;
        core::Amount balance;
    };

    struct GenesisConfig {
        static constexpr const char* kSection = "genesis";

        std::uint64_t timestamp{};
        std::vector<GenesisAccount> allocations;
        core::Bytes consensusSeal;

        static core::Result<GenesisConfig> from(const Config::Section& section);
    };

}

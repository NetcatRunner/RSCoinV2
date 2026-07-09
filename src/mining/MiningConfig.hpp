#pragma once

#include "config/Section.hpp"
#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Mining {

    struct MiningConfig {
        static constexpr const char* kSection = "mining";

        bool enabled{};
        core::Address beneficiary;

        static core::Result<MiningConfig> from(const Config::Section& section);
    };

}

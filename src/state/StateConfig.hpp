#pragma once

#include "config/Section.hpp"
#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::State {

    struct StateConfig {
        static constexpr const char* kSection = "state";

        core::Amount blockReward;

        static core::Result<StateConfig> from(const Config::Section& section);
    };

}

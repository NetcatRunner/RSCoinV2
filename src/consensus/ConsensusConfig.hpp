#pragma once

#include <map>
#include <string>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Consensus {

    struct ConsensusConfig {
        static constexpr const char* kSection = "consensus";

        std::string engine;
        std::map<std::string, std::string> parameters;

        static core::Result<ConsensusConfig> from(const Config::Section& section);
    };

}

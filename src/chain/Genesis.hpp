#pragma once

#include "config/NodeConfig.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Chain {
    Primitives::Block buildGenesis(const Config::GenesisConfig& config, const core::Hash256& stateRoot);
}

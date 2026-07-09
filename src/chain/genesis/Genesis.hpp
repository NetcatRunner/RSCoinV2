#pragma once

#include "chain/GenesisConfig.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Chain {
    Primitives::Block buildGenesis(const GenesisConfig& config, const core::Hash256& stateRoot);
}

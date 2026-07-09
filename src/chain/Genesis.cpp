#include "chain/Genesis.hpp"

namespace RSCoin::Chain {

    Primitives::Block buildGenesis(const GenesisConfig& config, const core::Hash256& stateRoot) {
        Primitives::Block genesis;
        genesis.header.height = 0;
        genesis.header.timestamp = config.timestamp;
        genesis.header.stateRoot = stateRoot;
        genesis.header.consensusSeal = config.consensusSeal;
        return genesis;
    }

}

#pragma once

#include <memory>

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "consensus/IConsensus.hpp"
#include "crypto/ICrypto.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Chain {

    core::Result<std::unique_ptr<IBlockchain>> makeBlockchain(Storage::IKeyValueStore& store, const Crypto::IHasher& hasher, const Primitives::Block& genesis);

    struct ChainManagerDeps {
        IBlockchain& chain;
        const Consensus::IConsensus& consensus;
        const Crypto::IHasher& hasher;
    };

    core::Result<std::unique_ptr<IChainManager>> makeChainManager(ChainManagerDeps deps, std::unique_ptr<State::IStateMachine> genesisState);

}

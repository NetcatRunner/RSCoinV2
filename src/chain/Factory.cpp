#include "chain/Factory.hpp"

#include "chain/kv/Blockchain.hpp"
#include "chain/manager/ChainManager.hpp"

namespace RSCoin::Chain {

    core::Result<std::unique_ptr<IChainManager>> makeChainManager(ChainManagerDeps deps, std::unique_ptr<State::IStateMachine> genesisState) {
        auto manager = ChainManager::create(ChainManager::Dependencies{deps.chain, deps.consensus, deps.hasher}, std::move(genesisState));
        if (!manager)
            return core::fail(manager.error());
        return std::unique_ptr<IChainManager>(std::move(*manager));
    }

    core::Result<std::unique_ptr<IBlockchain>> makeBlockchain(Storage::IKeyValueStore& store, const Crypto::IHasher& hasher, const Primitives::Block& genesis) {
        auto chain = Blockchain::open(store, hasher, genesis);
        if (!chain)
            return core::fail(chain.error());
        return std::unique_ptr<IBlockchain>(std::move(*chain));
    }

}

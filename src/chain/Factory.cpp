#include "chain/Factory.hpp"

#include "chain/kv/Blockchain.hpp"
#include "chain/manager/ChainManager.hpp"

namespace RSCoin::Chain {

    core::Result<std::unique_ptr<IChainManager>> makeChainManager(ChainManagerDeps deps, std::unique_ptr<State::IStateMachine> genesisState) {

        std::unique_ptr<State::IStateMachine> ledger = std::move(genesisState);
        for (std::uint64_t height = 1; height <= deps.chain.height(); ++height) {
            auto block = deps.chain.blockByHeight(height);
            if (!block)
                return core::fail(block.error(), "replaying chain at height " + std::to_string(height));

            auto next = ledger->apply(*block);
            if (!next)
                return core::fail(next.error(), "replaying block " + std::to_string(height));
            if ((*next)->stateRoot() != block->header.stateRoot)
                return core::fail(core::ErrorCode::state, "state root mismatch while replaying block " + std::to_string(height));
            ledger = std::move(*next);
        }

        return std::make_unique<ChainManager>(ChainManager::Dependencies{deps.chain, deps.consensus, deps.hasher}, std::move(ledger));
    }

    core::Result<std::unique_ptr<IBlockchain>> makeBlockchain(Storage::IKeyValueStore& store, const Crypto::IHasher& hasher, const Primitives::Block& genesis) {
        auto chain = Blockchain::open(store, hasher, genesis);
        if (!chain)
            return core::fail(chain.error());
        return std::unique_ptr<IBlockchain>(std::move(*chain));
    }

}

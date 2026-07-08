#pragma once

#include <mutex>
#include <vector>

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "consensus/IConsensus.hpp"
#include "crypto/ICrypto.hpp"

namespace RSCoin::Chain {

    class ChainManager : public IChainManager {
    public:
        struct Dependencies {
            IBlockchain& chain;
            const Consensus::IConsensus& consensus;
            const Crypto::IHasher& hasher;
        };

        ChainManager(Dependencies dependencies, std::unique_ptr<State::IStateMachine> genesisState);

        core::Result<ImportOutcome> importBlock(const Primitives::Block& block, ImportOrigin origin) override;
        std::shared_ptr<const State::IStateMachine> ledger() const override;
        void subscribe(BlockSubscriber subscriber) override;

    private:
        core::Result<ImportOutcome> importLocked(const Primitives::Block& block);

        Dependencies _deps;

        mutable std::mutex _mutex;
        std::shared_ptr<const State::IStateMachine> _ledger;
        std::vector<BlockSubscriber> _subscribers;
    };

}

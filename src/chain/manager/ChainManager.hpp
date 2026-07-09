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

        static core::Result<std::unique_ptr<ChainManager>> create(Dependencies dependencies, std::unique_ptr<State::IStateMachine> genesisState);

        core::Result<ImportOutcome> importBlock(const Primitives::Block& block, ImportOrigin origin) override;
        std::shared_ptr<const State::IStateMachine> ledger() const override;
        void subscribe(BlockSubscriber subscriber) override;

    private:
        using Ledger = std::shared_ptr<const State::IStateMachine>;

        ChainManager(Dependencies dependencies, Ledger genesis)
            : _deps(dependencies), _genesisLedger(std::move(genesis)) {}

        core::Result<ImportOutcome> importLocked(const Primitives::Block& block);
        core::Result<ImportOutcome> reorgLocked(const Primitives::Block& tip);

        core::Result<Ledger> replayBranch(const std::vector<Primitives::Block>& branch) const;

        Dependencies _deps;
        Ledger _genesisLedger;

        mutable std::mutex _mutex;
        Ledger _ledger;
        std::vector<BlockSubscriber> _subscribers;
    };

}

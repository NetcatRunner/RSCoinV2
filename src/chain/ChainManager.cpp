#include "chain/ChainManager.hpp"

#include <utility>

#include "log/Logger.hpp"
#include "primitives/Codec.hpp"

namespace RSCoin::Chain {

    ChainManager::ChainManager(Dependencies dependencies, std::unique_ptr<State::IStateMachine> genesisState)
        : _deps(dependencies), _ledger(std::move(genesisState)) {}

    std::shared_ptr<const State::IStateMachine> ChainManager::ledger() const {
        std::lock_guard lock(_mutex);
        return _ledger;
    }

    void ChainManager::subscribe(BlockSubscriber subscriber) {
        std::lock_guard lock(_mutex);
        _subscribers.push_back(std::move(subscriber));
    }

    core::Result<ImportOutcome> ChainManager::importBlock(const Primitives::Block& block, ImportOrigin origin) {
        core::Result<ImportOutcome> outcome;
        std::vector<BlockSubscriber> subscribers;
        {
            std::lock_guard lock(_mutex);
            outcome = importLocked(block);
            if (outcome && *outcome == ImportOutcome::imported)
                subscribers = _subscribers;
        }

        for (const auto& subscriber : subscribers)
            subscriber(block, origin);
        return outcome;
    }

    core::Result<ImportOutcome> ChainManager::importLocked(const Primitives::Block& block) {
        const core::Hash256 hash = _deps.hasher.hash(Primitives::encode(block.header));

        if (_deps.chain.headerByHash(hash))
            return ImportOutcome::alreadyKnown;

        if (!_deps.chain.headerByHash(block.header.parentHash))
            return ImportOutcome::orphaned;

        if (block.header.parentHash != _deps.chain.headHash()) {
            RSCoin_WARN("fork at height {} ignored (reorgs not supported yet)", block.header.height);
            return ImportOutcome::forked;
        }

        if (auto verified = _deps.consensus.verify(block.header, _deps.chain); !verified) {
            RSCoin_WARN("block {} rejected by consensus: {}", block.header.height, verified.error().message);
            return ImportOutcome::invalid;
        }

        auto next = _ledger->apply(block);
        if (!next) {
            RSCoin_WARN("block {} failed execution: {}", block.header.height, next.error().message);
            return ImportOutcome::invalid;
        }
        if ((*next)->stateRoot() != block.header.stateRoot) {
            RSCoin_WARN("block {} state root mismatch", block.header.height);
            return ImportOutcome::invalid;
        }

        if (auto appended = _deps.chain.appendBlock(block); !appended)
            return core::fail(appended.error(), "extending chain");

        _ledger = std::shared_ptr<const State::IStateMachine>(std::move(*next));
        return ImportOutcome::imported;
    }

}

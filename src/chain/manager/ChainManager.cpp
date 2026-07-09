#include "chain/manager/ChainManager.hpp"

#include <algorithm>
#include <utility>

#include "log/Logger.hpp"
#include "primitives/Codec.hpp"

namespace RSCoin::Chain {

    core::Result<std::unique_ptr<ChainManager>> ChainManager::create(Dependencies dependencies, std::unique_ptr<State::IStateMachine> genesisState) {
        auto manager = std::unique_ptr<ChainManager>(new ChainManager(dependencies, Ledger(std::move(genesisState))));

        auto ledger = manager->replayBranch({});
        if (!ledger)
            return core::fail(ledger.error(), "replaying chain");
        manager->_ledger = std::move(*ledger);
        return manager;
    }

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
            if (outcome && (*outcome == ImportOutcome::imported || *outcome == ImportOutcome::reorged))
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

        if (auto verified = _deps.consensus.verify(block.header, _deps.chain); !verified) {
            RSCoin_WARN("block {} rejected by consensus: {}", block.header.height, verified.error().message);
            return ImportOutcome::invalid;
        }

        if (block.header.parentHash == _deps.chain.headHash()) {
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

            _ledger = Ledger(std::move(*next));
            return ImportOutcome::imported;
        }

        if (auto stored = _deps.chain.storeBlock(block); !stored)
            return core::fail(stored.error(), "storing side-chain block");

        const auto order = _deps.consensus.compare(block.header, _deps.chain.head(), _deps.chain);
        if (!order)
            return core::fail(order.error(), "comparing forks");
        if (*order != std::strong_ordering::greater) {
            RSCoin_INFO("block {} kept on a side chain (current head still best)", block.header.height);
            return ImportOutcome::forked;
        }
        return reorgLocked(block);
    }

    core::Result<ImportOutcome> ChainManager::reorgLocked(const Primitives::Block& tip) {
        auto reachedCanonicalTrunk = [this](const Primitives::BlockHeader& lowest) {
            auto canonical = _deps.chain.headerByHeight(lowest.height - 1);
            return canonical && _deps.hasher.hash(Primitives::encode(*canonical)) == lowest.parentHash;
        };

        std::vector<Primitives::Block> branch{tip};
        while (!reachedCanonicalTrunk(branch.back().header)) {
            auto parent = _deps.chain.blockByHash(branch.back().header.parentHash);
            if (!parent)
                return core::fail(parent.error(), "walking fork branch");
            branch.push_back(std::move(*parent));
        }
        std::reverse(branch.begin(), branch.end());

        auto ledger = replayBranch(branch);
        if (!ledger) {
            RSCoin_WARN("reorg to height {} aborted: {}", tip.header.height, ledger.error().message);
            return ImportOutcome::invalid;
        }

        auto adopted = _deps.chain.adoptBranch(branch);
        if (!adopted)
            return core::fail(adopted.error(), "reorg");
        _ledger = std::move(*ledger);

        RSCoin_INFO("reorg: adopted better chain at height {} ({} block(s) replaced from height {})", tip.header.height, branch.size(), branch.front().header.height);
        return ImportOutcome::reorged;
    }

    core::Result<ChainManager::Ledger> ChainManager::replayBranch(const std::vector<Primitives::Block>& branch) const {
        const std::uint64_t forkHeight = branch.empty() ? _deps.chain.height() : branch.front().header.height - 1;

        Ledger ledger = _genesisLedger;
        auto step = [&ledger](const Primitives::Block& block) -> core::Result<void> {
            auto next = ledger->apply(block);
            if (!next)
                return core::fail(next.error(), "block " + std::to_string(block.header.height));
            if ((*next)->stateRoot() != block.header.stateRoot)
                return core::fail(core::ErrorCode::state, "state root mismatch at block " + std::to_string(block.header.height));
            ledger = Ledger(std::move(*next));
            return {};
        };

        for (std::uint64_t height = 1; height <= forkHeight; ++height) {
            auto block = _deps.chain.blockByHeight(height);
            if (!block)
                return core::fail(block.error(), "replaying chain");
            if (auto applied = step(*block); !applied)
                return core::fail(applied.error());
        }
        for (const auto& block : branch) {
            if (auto applied = step(block); !applied)
                return core::fail(applied.error());
        }
        return ledger;
    }

}

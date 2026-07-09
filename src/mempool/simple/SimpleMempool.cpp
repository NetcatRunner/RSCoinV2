#include "mempool/simple/SimpleMempool.hpp"

#include <algorithm>

#include "log/Logger.hpp"

namespace RSCoin::Mempool {

    SimpleMempool::SimpleMempool(Chain::IChainManager& manager): _manager(manager) {}

    core::Result<AddOutcome> SimpleMempool::add(Primitives::Transaction transaction) {
        if (const auto valid = _manager.ledger()->validateTransaction(transaction); !valid) {
            RSCoin_WARN("transaction rejected: {}", valid.error().describe());
            return AddOutcome::rejected;
        }

        std::vector<TxSubscriber> subscribers;
        {
            std::lock_guard lock(_mutex);
            if (std::ranges::any_of(_transactions, [&](const auto& known) { return known.from == transaction.from && known.nonce == transaction.nonce; })) {
                return AddOutcome::alreadyKnown;
            } 
            _transactions.push_back(transaction);
            subscribers = _subscribers;
            RSCoin_INFO("transaction admitted (pool size {})", _transactions.size());
        }

        for (const auto& subscriber : subscribers)
            subscriber(transaction);
        return AddOutcome::added;
    }

    std::vector<Primitives::Transaction> SimpleMempool::take(std::size_t maxCount) {
        const auto ledger = _manager.ledger();

        std::lock_guard lock(_mutex);
        std::erase_if(_transactions, [&](const auto& known) {
            const auto valid = ledger->validateTransaction(known);
            if (!valid)
                RSCoin_WARN("evicting stale transaction: {}", valid.error().describe());
            return !valid.has_value();
        });

        const std::size_t count = std::min(maxCount, _transactions.size());
        return {_transactions.begin(), _transactions.begin() + static_cast<std::ptrdiff_t>(count)};
    }

    void SimpleMempool::removeIncluded(const Primitives::Block& block) {
        std::lock_guard lock(_mutex);
        std::erase_if(_transactions, [&](const auto& known) {
            return std::ranges::any_of(block.transactions, [&](const auto& included) {
                return included.from == known.from && included.nonce == known.nonce;
            });
        });
    }

    std::size_t SimpleMempool::size() const noexcept {
        std::lock_guard lock(_mutex);
        return _transactions.size();
    }

    void SimpleMempool::subscribe(TxSubscriber subscriber) {
        std::lock_guard lock(_mutex);
        _subscribers.push_back(std::move(subscriber));
    }

}

#include "mempool/SimpleMempool.hpp"

#include <algorithm>

namespace RSCoin::Mempool {

    core::Result<void> SimpleMempool::add(Primitives::Transaction transaction) {
        std::lock_guard lock(_mutex);

        const bool duplicate = std::ranges::any_of(_transactions, [&](const auto& known) {
            return known.from == transaction.from && known.nonce == transaction.nonce;
        });
        if (duplicate)
            return core::fail(core::ErrorCode::validation, "transaction already in mempool");

        _transactions.push_back(std::move(transaction));
        return {};
    }

    std::vector<Primitives::Transaction> SimpleMempool::take(std::size_t maxCount) {
        std::lock_guard lock(_mutex);
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

}

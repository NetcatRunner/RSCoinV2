#pragma once

#include <mutex>
#include <vector>

#include "mempool/IMempool.hpp"

namespace RSCoin::Mempool {

    class SimpleMempool : public IMempool {
    public:
        core::Result<void> add(Primitives::Transaction transaction) override;
        std::vector<Primitives::Transaction> take(std::size_t maxCount) override;
        void removeIncluded(const Primitives::Block& block) override;
        std::size_t size() const noexcept override;

    private:
        mutable std::mutex _mutex;
        std::vector<Primitives::Transaction> _transactions;
    };

}

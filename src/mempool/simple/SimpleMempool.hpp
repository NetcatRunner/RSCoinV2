#pragma once

#include <mutex>
#include <vector>

#include "chain/IChainManager.hpp"
#include "mempool/IMempool.hpp"

namespace RSCoin::Mempool {

    class SimpleMempool : public IMempool {
    public:
        SimpleMempool(Chain::IChainManager& manager);

        core::Result<AddOutcome> add(Primitives::Transaction transaction) override;
        std::vector<Primitives::Transaction> take(std::size_t maxCount) override;
        void removeIncluded(const Primitives::Block& block) override;
        std::size_t size() const noexcept override;

        void subscribe(TxSubscriber subscriber) override;

    private:

        Chain::IChainManager& _manager;

        mutable std::mutex _mutex;
        std::vector<Primitives::Transaction> _transactions;
        std::vector<TxSubscriber> _subscribers;
    };

}

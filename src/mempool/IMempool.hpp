#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "core/Result.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Mempool {

    enum class AddOutcome {
        added,
        alreadyKnown,
        rejected,
    };

    class IMempool {
    public:
        using TxSubscriber = std::function<void(const Primitives::Transaction&)>;

        virtual ~IMempool() = default;

        virtual core::Result<AddOutcome> add(Primitives::Transaction transaction) = 0;
        virtual std::vector<Primitives::Transaction> take(std::size_t maxCount) = 0;
        virtual void removeIncluded(const Primitives::Block& block) = 0;
        virtual std::size_t size() const noexcept = 0;

        virtual void subscribe(TxSubscriber subscriber) = 0;
    };

}

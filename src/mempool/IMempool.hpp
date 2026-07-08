#pragma once

#include <cstddef>
#include <vector>

#include "core/Result.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Mempool {

    class IMempool {
    public:
        virtual ~IMempool() = default;

        virtual core::Result<void> add(Primitives::Transaction transaction) = 0;
        virtual std::vector<Primitives::Transaction> take(std::size_t maxCount) = 0;
        virtual void removeIncluded(const Primitives::Block& block) = 0;
        virtual std::size_t size() const noexcept = 0;
    };

}

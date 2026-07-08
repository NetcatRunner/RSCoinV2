#pragma once

#include <cstdint>

#include "core/Result.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Chain {

    class IChainView {
    public:
        virtual ~IChainView() = default;

        virtual Primitives::BlockHeader head() const = 0;
        virtual core::Result<Primitives::BlockHeader> headerByHash(const core::Hash256& hash) const = 0;
        virtual core::Result<Primitives::BlockHeader> headerByHeight(std::uint64_t height) const = 0;
    };

}

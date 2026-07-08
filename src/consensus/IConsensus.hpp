#pragma once

#include <compare>
#include <stop_token>
#include <string_view>

#include "core/Result.hpp"
#include "chain/IChainView.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Consensus {

    class IConsensus {
    public:
        virtual ~IConsensus() = default;

        virtual std::string_view name() const noexcept = 0;

        virtual core::Result<void> verify(const Primitives::BlockHeader& header, const Chain::IChainView& chain) const = 0;

        virtual core::Result<void> prepare(Primitives::BlockHeader& draft, const Chain::IChainView& chain) const = 0;

        virtual core::Result<Primitives::Block> seal(Primitives::Block draft, const Chain::IChainView& chain, std::stop_token cancel) const = 0;

        virtual core::Result<std::strong_ordering> compare(const Primitives::BlockHeader& lhs, const Primitives::BlockHeader& rhs, const Chain::IChainView& chain) const = 0;
    };

}

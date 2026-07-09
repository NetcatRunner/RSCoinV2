#pragma once

#include <vector>

#include "chain/IChainView.hpp"

namespace RSCoin::Chain {

    class IBlockchain : public IChainView {
    public:
        virtual core::Result<void> appendBlock(const Primitives::Block& block) = 0;
        virtual core::Result<void> storeBlock(const Primitives::Block& block) = 0;
        virtual core::Result<void> adoptBranch(const std::vector<Primitives::Block>& branch) = 0;

        virtual core::Result<Primitives::Block> blockByHash(const core::Hash256& hash) const = 0;
        virtual core::Result<Primitives::Block> blockByHeight(std::uint64_t height) const = 0;

        virtual core::Hash256 headHash() const = 0;
        virtual std::uint64_t height() const = 0;
    };

}

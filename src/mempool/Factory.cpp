#include "mempool/Factory.hpp"

#include "mempool/simple/SimpleMempool.hpp"

namespace RSCoin::Mempool {

    core::Result<std::unique_ptr<IMempool>> makeMempool(Chain::IChainManager& manager) {
        auto pool = std::make_unique<SimpleMempool>(manager);

        manager.subscribe([raw = pool.get()](const Primitives::Block& block, Chain::ImportOrigin) {
            raw->removeIncluded(block);
        });
        return pool;
    }

}

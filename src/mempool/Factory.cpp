#include "mempool/Factory.hpp"

#include "mempool/SimpleMempool.hpp"

namespace RSCoin::Mempool {
    core::Result<std::unique_ptr<IMempool>> makeMempool() {
        return std::make_unique<SimpleMempool>();
    }

}

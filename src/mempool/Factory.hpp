#pragma once

#include <memory>

#include "core/Result.hpp"
#include "mempool/IMempool.hpp"

namespace RSCoin::Mempool {
    core::Result<std::unique_ptr<IMempool>> makeMempool();
}

#pragma once

#include <memory>

#include "chain/IChainManager.hpp"
#include "core/Result.hpp"
#include "mempool/IMempool.hpp"

namespace RSCoin::Mempool {

    core::Result<std::unique_ptr<IMempool>> makeMempool(Chain::IChainManager& manager);
}

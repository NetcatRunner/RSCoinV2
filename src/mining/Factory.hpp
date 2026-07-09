#pragma once

#include <memory>

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "chain/BlockConfig.hpp"
#include "mining/MiningConfig.hpp"
#include "consensus/IConsensus.hpp"
#include "core/Result.hpp"
#include "mempool/IMempool.hpp"
#include "mining/IMiner.hpp"

namespace RSCoin::Mining {

    struct MinerDeps {
        Chain::IBlockchain& chain;
        const Consensus::IConsensus& consensus;
        Chain::IChainManager& manager;
        Mempool::IMempool& mempool;
    };

    core::Result<std::unique_ptr<IMiner>> makeMiner(MinerDeps deps, MiningConfig mining, Chain::BlockConfig block);

}

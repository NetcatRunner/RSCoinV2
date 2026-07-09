#include "mining/Factory.hpp"

#include "mining/Miner.hpp"

namespace RSCoin::Mining {

    core::Result<std::unique_ptr<IMiner>> makeMiner(MinerDeps deps, MiningConfig mining, Chain::BlockConfig block) {
        return std::make_unique<Miner>(Miner::Dependencies{deps.chain, deps.consensus, deps.manager, deps.mempool}, std::move(mining), std::move(block));
    }

}

#pragma once

#include <mutex>
#include <stop_token>
#include <thread>

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "chain/BlockConfig.hpp"
#include "mining/MiningConfig.hpp"
#include "consensus/IConsensus.hpp"
#include "mempool/IMempool.hpp"
#include "mining/IMiner.hpp"

namespace RSCoin::Mining {

    class Miner : public IMiner {
    public:
        struct Dependencies {
            Chain::IBlockchain& chain;
            const Consensus::IConsensus& consensus;
            Chain::IChainManager& manager;
            Mempool::IMempool& mempool;
        };

        Miner(Dependencies dependencies, MiningConfig mining, Chain::BlockConfig block);
        ~Miner() override;

        void start() override;
        void stop() override;

    private:
        void miningLoop(std::stop_token cancel);
        core::Result<void> mineOne();

        void interruptSeal();
        std::stop_token armSealInterrupt();

        Dependencies _deps;
        MiningConfig _mining;
        Chain::BlockConfig _block;

        std::stop_source _stopSource;
        std::thread _thread;

        std::mutex _sealMutex;
        std::stop_source _sealInterrupt;
    };

}

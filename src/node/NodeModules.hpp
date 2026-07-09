#pragma once

#include <memory>

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "core/Result.hpp"
#include "consensus/IConsensus.hpp"
#include "crypto/ICrypto.hpp"
#include "mempool/IMempool.hpp"
#include "mining/IMiner.hpp"
#include "network/INetwork.hpp"
#include "protocol/IProtocol.hpp"
#include "rpc/IRpcServer.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Node {

    struct Modules {
        std::unique_ptr<Crypto::ICryptoProvider> crypto;
        std::unique_ptr<Storage::IKeyValueStore> storage;
        std::unique_ptr<Chain::IBlockchain> chain;
        std::unique_ptr<Consensus::IConsensus> consensus;
        std::unique_ptr<Mempool::IMempool> mempool;
        std::unique_ptr<Chain::IChainManager> manager;
        std::unique_ptr<Network::INetwork> network;
        std::unique_ptr<Protocol::IProtocol> protocol;
        std::unique_ptr<Rpc::IRpcServer> rpc;
        std::unique_ptr<Mining::IMiner> miner;
    };
}

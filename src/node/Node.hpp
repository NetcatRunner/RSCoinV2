#pragma once

#include <memory>
#include <stop_token>

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "config/NodeConfig.hpp"
#include "core/Result.hpp"
#include "consensus/IConsensus.hpp"
#include "crypto/ICrypto.hpp"
#include "mempool/IMempool.hpp"
#include "network/INetwork.hpp"
#include "protocol/IProtocol.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Node {

    struct Modules {
        std::unique_ptr<Crypto::ICryptoProvider> crypto;
        std::unique_ptr<Storage::IKeyValueStore> storage;
        std::unique_ptr<Chain::IBlockchain> chain;
        std::unique_ptr<Consensus::IConsensus> consensus;
        std::unique_ptr<Chain::IChainManager> manager;
        std::unique_ptr<Network::INetwork> network;
        std::unique_ptr<Protocol::IProtocol> protocol;
        std::unique_ptr<Mempool::IMempool> mempool;
    };

    class Node {
    public:
        Node(Modules modules, Config::NodeConfig config);

        core::Result<void> run(std::stop_token stop);

    private:
        core::Result<void> mineOne(std::stop_token stop);

        Modules _modules;
        Config::NodeConfig _config;
    };

}

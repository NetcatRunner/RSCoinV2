#pragma once

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "crypto/ICrypto.hpp"
#include "mempool/IMempool.hpp"
#include "network/INetwork.hpp"

namespace RSCoin::Rpc {

    struct NodeServices {
        Chain::IBlockchain& chain;
        Chain::IChainManager& manager;
        Mempool::IMempool& mempool;
        Network::INetwork& network;
        const Crypto::IHasher& hasher;
    };

}

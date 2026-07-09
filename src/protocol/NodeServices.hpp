#pragma once

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "crypto/ICrypto.hpp"
#include "mempool/IMempool.hpp"

namespace RSCoin::Protocol {

    struct NodeServices {
        Chain::IBlockchain& chain;
        Chain::IChainManager& manager;
        const Crypto::IHasher& hasher;
        Mempool::IMempool& mempool;
    };

}

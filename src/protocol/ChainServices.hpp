#pragma once

#include "chain/IBlockchain.hpp"
#include "chain/IChainManager.hpp"
#include "crypto/ICrypto.hpp"

namespace RSCoin::Protocol {

    struct ChainServices {
        Chain::IBlockchain& chain;
        Chain::IChainManager& manager;
        const Crypto::IHasher& hasher;
    };

}

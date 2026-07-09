#pragma once

#include <memory>

#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"
#include "storage/IStorage.hpp"
#include "wallet/IWallet.hpp"

namespace RSCoin::Wallet {

    struct WalletDeps {
        const Crypto::ICryptoProvider& crypto;
        Storage::IKeyValueStore& keystore;
        std::uint64_t chainId{};
    };

    core::Result<std::unique_ptr<IWallet>> makeWallet(WalletDeps deps);

}

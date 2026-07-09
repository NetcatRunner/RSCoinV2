#pragma once

#include <memory>

#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"
#include "rpc/INodeApi.hpp"
#include "storage/IStorage.hpp"
#include "wallet/IWallet.hpp"
#include "wallet/IWalletUi.hpp"
#include "wallet/WalletConfig.hpp"
#include "wallet/terminal/TerminalUi.hpp"

namespace RSCoin::Wallet {

    struct WalletDeps {
        const Crypto::ICryptoProvider& crypto;
        Storage::IKeyValueStore& keystore;
        std::uint64_t chainId{};
    };

    core::Result<std::unique_ptr<IWallet>> makeWallet(WalletDeps deps);

    struct WalletUiDeps {
        IWallet& wallet;
        Rpc::INodeApi& node;
    };

    core::Result<std::unique_ptr<IWalletUi>> makeWalletUi(WalletUiDeps deps, const WalletConfig& config, Command command);

}

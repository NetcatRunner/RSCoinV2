#include "wallet/Factory.hpp"

#include "wallet/local/Wallet.hpp"

namespace RSCoin::Wallet {

    core::Result<std::unique_ptr<IWallet>> makeWallet(WalletDeps deps) {
        return std::make_unique<Wallet>(deps.crypto, Keystore(deps.keystore), deps.chainId);
    }

}

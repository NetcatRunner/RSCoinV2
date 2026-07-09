#include "wallet/Factory.hpp"

#include "wallet/local/Wallet.hpp"
#include "wallet/web/WebUi.hpp"

namespace RSCoin::Wallet {

    core::Result<std::unique_ptr<IWallet>> makeWallet(WalletDeps deps) {
        return std::make_unique<Wallet>(deps.crypto, Keystore(deps.keystore), deps.chainId);
    }

    core::Result<std::unique_ptr<IWalletUi>> makeWalletUi(WalletUiDeps deps, const WalletConfig& config, Command command) {
        if (!command.empty() || config.interface == "terminal")
            return std::make_unique<TerminalUi>(deps.wallet, deps.node, std::move(command));
        if (config.interface == "web")
            return std::make_unique<WebUi>(deps.wallet, deps.node, config.uiPort);

        return core::fail(core::ErrorCode::config, "unknown wallet interface: '" + config.interface + "'");
    }

}

#include "wallet/WalletConfig.hpp"

namespace RSCoin::Wallet {

    core::Result<WalletConfig> WalletConfig::from(const Config::Section& section) {
        WalletConfig config;
        std::string keystoreDirectory;

        Config::Reader reader(section);
        reader.read("keystoreDirectory", keystoreDirectory);
        reader.read("interface", config.interface);
        if (config.interface == "web")
            reader.read("uiPort", config.uiPort);

        config.keystoreDirectory = std::move(keystoreDirectory);
        return reader.finish(std::move(config));
    }

}

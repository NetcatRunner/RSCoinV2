#include "crypto/CryptoConfig.hpp"

namespace RSCoin::Crypto {

    core::Result<CryptoConfig> CryptoConfig::from(const Config::Section& section) {
        CryptoConfig config;
        Config::Reader reader(section);
        reader.read("hasher", config.hasher);
        reader.read("signatureScheme", config.signatureScheme);
        return reader.finish(std::move(config));
    }

}

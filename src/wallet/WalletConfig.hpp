#pragma once

#include <filesystem>
#include <string>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Wallet {

    struct WalletConfig {
        static constexpr const char* kSection = "wallet";

        std::filesystem::path keystoreDirectory;

        static core::Result<WalletConfig> from(const Config::Section& section);
    };

}

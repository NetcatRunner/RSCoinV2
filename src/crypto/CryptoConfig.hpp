#pragma once

#include <string>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Crypto {

    struct CryptoConfig {
        static constexpr const char* kSection = "crypto";

        std::string hasher;
        std::string signatureScheme;

        static core::Result<CryptoConfig> from(const Config::Section& section);
    };

}

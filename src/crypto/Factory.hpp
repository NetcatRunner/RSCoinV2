#pragma once

#include <memory>

#include "crypto/CryptoConfig.hpp"
#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {

    core::Result<std::unique_ptr<ICryptoProvider>> makeProvider(const CryptoConfig& config);

}

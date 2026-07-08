#pragma once

#include <memory>

#include "config/NodeConfig.hpp"
#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"
#include "state/IStateMachine.hpp"

namespace RSCoin::State {

    core::Result<std::unique_ptr<IStateMachine>> makeStateMachine(const Crypto::ICryptoProvider& crypto, const Config::NodeConfig& config);
}

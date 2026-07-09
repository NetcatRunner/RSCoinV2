#pragma once

#include <memory>

#include "chain/GenesisConfig.hpp"
#include "state/StateConfig.hpp"
#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"
#include "state/IStateMachine.hpp"

namespace RSCoin::State {

    core::Result<std::unique_ptr<IStateMachine>> makeStateMachine(const Crypto::ICryptoProvider& crypto, const StateConfig& rules, const Chain::GenesisConfig& genesis);
}

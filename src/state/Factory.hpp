#pragma once

#include <memory>

#include "chain/GenesisConfig.hpp"
#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"
#include "state/IStateMachine.hpp"
#include "state/StateConfig.hpp"

namespace RSCoin::State {

    core::Result<std::unique_ptr<IStateMachine>> makeStateMachine(const Crypto::ICryptoProvider& crypto, std::uint64_t chainId, const StateConfig& rules, const Chain::GenesisConfig& genesis);
}

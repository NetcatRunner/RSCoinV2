#pragma once

#include <memory>

#include "consensus/ConsensusConfig.hpp"
#include "consensus/IConsensus.hpp"
#include "core/Result.hpp"
#include "crypto/ICrypto.hpp"

namespace RSCoin::Consensus {

    core::Result<std::unique_ptr<IConsensus>> makeEngine(const ConsensusConfig& config, const Crypto::ICryptoProvider& crypto);

}

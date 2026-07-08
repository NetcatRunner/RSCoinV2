#pragma once

#include <memory>

#include "config/NodeConfig.hpp"
#include "core/Result.hpp"
#include "consensus/IConsensus.hpp"
#include "crypto/ICrypto.hpp"

namespace RSCoin::Consensus {
    core::Result<std::unique_ptr<Consensus::IConsensus>> makeEngine(const Config::ConsensusConfig& config, const Crypto::ICryptoProvider& crypto);
}

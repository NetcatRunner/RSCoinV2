#pragma once

#include <memory>

#include "network/NetworkConfig.hpp"
#include "core/Result.hpp"
#include "network/INetwork.hpp"

namespace RSCoin::Network {
    core::Result<std::unique_ptr<INetwork>> makeNetwork(const NetworkConfig& config);
}

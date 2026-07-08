#pragma once

#include <memory>

#include "config/NodeConfig.hpp"
#include "core/Result.hpp"
#include "network/INetwork.hpp"
#include "protocol/ChainServices.hpp"
#include "protocol/IProtocol.hpp"

namespace RSCoin::Protocol {
    core::Result<std::unique_ptr<IProtocol>> makeProtocol(const Config::NodeConfig& config, Network::INetwork& network, ChainServices services);
}

#pragma once

#include <cstdint>
#include <memory>

#include "core/Result.hpp"
#include "network/INetwork.hpp"
#include "protocol/IProtocol.hpp"
#include "protocol/NodeServices.hpp"
#include "protocol/ProtocolConfig.hpp"

namespace RSCoin::Protocol {

    core::Result<std::unique_ptr<IProtocol>> makeProtocol(const ProtocolConfig& config, std::uint64_t chainId, Network::INetwork& network, NodeServices services);
}

#include "protocol/Factory.hpp"

#include "protocol/rscoin/RSCoinProtocol.hpp"

namespace RSCoin::Protocol {

    core::Result<std::unique_ptr<IProtocol>> makeProtocol(const Config::NodeConfig& config, Network::INetwork& network, ChainServices services) {
        if (config.protocol.name == "rscoin")
            return std::make_unique<RSCoinProtocol>(network, config, services);
        return core::fail(core::ErrorCode::protocol, "unknown protocol: '" + config.protocol.name + "'");
    }

}

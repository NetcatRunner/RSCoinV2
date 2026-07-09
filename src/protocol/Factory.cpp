#include "protocol/Factory.hpp"

#include "protocol/rscoin/RSCoinProtocol.hpp"

namespace RSCoin::Protocol {

    core::Result<std::unique_ptr<IProtocol>> makeProtocol(const ProtocolConfig& config, std::uint64_t chainId, Network::INetwork& network, NodeServices services) {
        if (config.name == "rscoin")
            return std::make_unique<RSCoinProtocol>(network, RSCoinProtocol::Settings{config.version, chainId}, services);

        return core::fail(core::ErrorCode::protocol, "unknown protocol: '" + config.name + "'");
    }

}

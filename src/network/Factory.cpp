#include "network/Factory.hpp"

#include "network/tcp/TcpNetwork.hpp"

namespace RSCoin::Network {

    core::Result<std::unique_ptr<INetwork>> makeNetwork(const NetworkConfig& config) {
        if (config.transport == "tcp")
            return std::make_unique<TcpNetwork>(config);

        return core::fail(core::ErrorCode::network, "unknown network transport: '" + config.transport + "'");
    }

}

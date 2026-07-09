#include "rpc/Factory.hpp"

#include "rpc/http/HttpRpcClient.hpp"
#include "rpc/http/HttpRpcServer.hpp"

namespace RSCoin::Rpc {

    namespace {
        // Un client ne peut pas composer 0.0.0.0 : écouter partout se joint en local.
        std::string dialHost(const RpcConfig& config) {
            return config.listenAddress == "0.0.0.0" ? "127.0.0.1" : config.listenAddress;
        }
    }

    core::Result<std::unique_ptr<IRpcServer>> makeRpcServer(RpcConfig config, NodeServices services) {
        if (config.transport == "http")
            return std::make_unique<HttpRpcServer>(std::move(config), services);

        return core::fail(core::ErrorCode::config, "unknown rpc transport: '" + config.transport + "'");
    }

    core::Result<std::unique_ptr<IRpcClient>> makeRpcClient(const RpcConfig& config) {
        if (!config.enabled)
            return core::fail(core::ErrorCode::config, "rpc is disabled in the configuration");

        if (config.transport == "http")
            return std::make_unique<HttpRpcClient>(dialHost(config), config.port);

        return core::fail(core::ErrorCode::config, "unknown rpc transport: '" + config.transport + "'");
    }

}

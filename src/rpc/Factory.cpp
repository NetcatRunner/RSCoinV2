#include "rpc/Factory.hpp"

#include "rpc/http/HttpClient.hpp"
#include "rpc/http/HttpServer.hpp"
#include "rpc/jsonrpc/JsonRpcClient.hpp"
#include "rpc/jsonrpc/JsonRpcHandler.hpp"
#include "rpc/node/NodeApi.hpp"

namespace RSCoin::Rpc {

    namespace {
        std::string dialHost(const RpcConfig& config) {
            return config.listenAddress == "0.0.0.0" ? "127.0.0.1" : config.listenAddress;
        }
    }

    core::Result<std::unique_ptr<IRpcServer>> makeRpcServer(RpcConfig config, NodeServices services) {
        auto handler = std::make_unique<JsonRpcHandler>(std::make_unique<NodeApi>(services));

        if (config.transport == "http")
            return std::make_unique<HttpServer>(std::move(config), std::move(handler));

        return core::fail(core::ErrorCode::config, "unknown rpc transport: '" + config.transport + "'");
    }

    core::Result<std::unique_ptr<INodeApi>> makeRpcClient(const RpcConfig& config) {
        if (!config.enabled)
            return core::fail(core::ErrorCode::config, "rpc is disabled in the configuration");

        std::unique_ptr<IClientTransport> transport;
        if (config.transport == "http")
            transport = std::make_unique<HttpClient>(dialHost(config), config.port);
        else
            return core::fail(core::ErrorCode::config, "unknown rpc transport: '" + config.transport + "'");

        return std::make_unique<JsonRpcClient>(std::move(transport));
    }

}

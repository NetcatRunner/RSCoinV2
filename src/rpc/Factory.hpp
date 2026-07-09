#pragma once

#include <memory>

#include "core/Result.hpp"
#include "rpc/IRpcClient.hpp"
#include "rpc/IRpcServer.hpp"
#include "rpc/NodeServices.hpp"
#include "rpc/RpcConfig.hpp"

namespace RSCoin::Rpc {

    core::Result<std::unique_ptr<IRpcServer>> makeRpcServer(RpcConfig config, NodeServices services);

    // Client assorti au serveur : même config, même transport.
    core::Result<std::unique_ptr<IRpcClient>> makeRpcClient(const RpcConfig& config);

}

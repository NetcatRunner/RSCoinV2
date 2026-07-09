#pragma once

#include <memory>

#include "core/Result.hpp"
#include "rpc/INodeApi.hpp"
#include "rpc/IRpcServer.hpp"
#include "rpc/NodeServices.hpp"
#include "rpc/RpcConfig.hpp"

namespace RSCoin::Rpc {

    core::Result<std::unique_ptr<IRpcServer>> makeRpcServer(RpcConfig config, NodeServices services);

    core::Result<std::unique_ptr<INodeApi>> makeRpcClient(const RpcConfig& config);

}

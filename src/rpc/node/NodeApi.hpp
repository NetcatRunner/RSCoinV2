#pragma once

#include "rpc/INodeApi.hpp"
#include "rpc/NodeServices.hpp"

namespace RSCoin::Rpc {

    class NodeApi : public INodeApi {
    public:
        NodeApi(NodeServices services) : _services(services) {}

        core::Result<SendReceipt> sendTransaction(const Primitives::Transaction& transaction) override;
        core::Result<AccountInfo> getAccount(const core::Address& address) override;
        core::Result<NodeStatus> status() override;

    private:
        NodeServices _services;
    };

}

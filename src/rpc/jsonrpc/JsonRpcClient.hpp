#pragma once

#include <cstdint>
#include <memory>

#include "rpc/INodeApi.hpp"
#include "rpc/ITransport.hpp"

namespace RSCoin::Rpc {

    class JsonRpcClient : public INodeApi {
    public:
        JsonRpcClient(std::unique_ptr<IClientTransport> transport) : _transport(std::move(transport)) {}

        core::Result<SendReceipt> sendTransaction(const Primitives::Transaction& transaction) override;
        core::Result<AccountInfo> getAccount(const core::Address& address) override;
        core::Result<NodeStatus> status() override;

    private:
        std::unique_ptr<IClientTransport> _transport;
        std::uint64_t _nextId{1};
    };

}

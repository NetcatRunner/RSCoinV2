#pragma once

#include "core/Result.hpp"
#include "primitives/Transaction.hpp"
#include "rpc/Api.hpp"

namespace RSCoin::Rpc {

    class INodeApi {
    public:
        virtual ~INodeApi() = default;

        virtual core::Result<SendReceipt> sendTransaction(const Primitives::Transaction& transaction) = 0;
        virtual core::Result<AccountInfo> getAccount(const core::Address& address) = 0;
        virtual core::Result<NodeStatus> status() = 0;
    };

}

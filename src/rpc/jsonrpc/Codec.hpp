#pragma once

#include <nlohmann/json_fwd.hpp>

#include "core/Result.hpp"
#include "primitives/Transaction.hpp"
#include "rpc/Api.hpp"

namespace RSCoin::Rpc {

    namespace Method {
        inline constexpr const char* kSendTransaction = "rscoin_sendRawTransaction";
        inline constexpr const char* kGetAccount = "rscoin_getAccount";
        inline constexpr const char* kStatus = "rscoin_status";
    }

    nlohmann::json paramsOf(const Primitives::Transaction& transaction);
    nlohmann::json paramsOf(const core::Address& address);
    core::Result<Primitives::Transaction> transactionParam(const nlohmann::json& params);
    core::Result<core::Address> addressParam(const nlohmann::json& params);

    nlohmann::json toJson(const SendReceipt& receipt);
    nlohmann::json toJson(const AccountInfo& account);
    nlohmann::json toJson(const NodeStatus& status);
    core::Result<SendReceipt> receiptFromJson(const nlohmann::json& result);
    core::Result<AccountInfo> accountFromJson(const nlohmann::json& result);
    core::Result<NodeStatus> statusFromJson(const nlohmann::json& result);

}

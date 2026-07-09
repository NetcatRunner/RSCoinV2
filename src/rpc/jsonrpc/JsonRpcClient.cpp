#include "rpc/jsonrpc/JsonRpcClient.hpp"

#include <nlohmann/json.hpp>

#include "rpc/jsonrpc/Codec.hpp"

namespace RSCoin::Rpc {

    namespace {
        core::Result<nlohmann::json> call(IClientTransport& transport, std::uint64_t id, const char* method, const nlohmann::json& params) {
            const nlohmann::json request{
                {"jsonrpc", "2.0"},
                {"id", id},
                {"method", method},
                {"params", params},
            };
            const auto body = transport.roundTrip(request.dump(), "application/json");
            if (!body)
                return core::fail(body.error());

            const auto response = nlohmann::json::parse(*body, nullptr, false);
            if (response.is_discarded() || !response.is_object())
                return core::fail(core::ErrorCode::protocol, "malformed rpc response");
            if (response.contains("error"))
                return core::fail(core::ErrorCode::protocol, response["error"].value("message", "unknown rpc error"));
            if (!response.contains("result"))
                return core::fail(core::ErrorCode::protocol, "rpc response without result");
            return response["result"];
        }
    }

    core::Result<SendReceipt> JsonRpcClient::sendTransaction(const Primitives::Transaction& transaction) {
        const auto result = call(*_transport, _nextId++, Method::kSendTransaction, paramsOf(transaction));
        if (!result)
            return core::fail(result.error());
        return receiptFromJson(*result);
    }

    core::Result<AccountInfo> JsonRpcClient::getAccount(const core::Address& address) {
        const auto result = call(*_transport, _nextId++, Method::kGetAccount, paramsOf(address));
        if (!result)
            return core::fail(result.error());
        return accountFromJson(*result);
    }

    core::Result<NodeStatus> JsonRpcClient::status() {
        const auto result = call(*_transport, _nextId++, Method::kStatus, nlohmann::json::array());
        if (!result)
            return core::fail(result.error());
        return statusFromJson(*result);
    }

}

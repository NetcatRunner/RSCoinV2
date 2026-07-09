#include "rpc/jsonrpc/JsonRpcHandler.hpp"

#include <nlohmann/json.hpp>

#include "rpc/jsonrpc/Codec.hpp"

namespace RSCoin::Rpc {

    namespace {
        std::string errorResponse(const nlohmann::json& id, int code, const std::string& message) {
            const nlohmann::json response{
                {"jsonrpc", "2.0"},
                {"id", id},
                {"error", {{"code", code}, {"message", message}}},
            };
            return response.dump();
        }

        core::Result<nlohmann::json> dispatch(INodeApi& api, std::string_view method, const nlohmann::json& params) {
            if (method == Method::kSendTransaction) {
                const auto transaction = transactionParam(params);
                if (!transaction)
                    return core::fail(transaction.error());
                const auto receipt = api.sendTransaction(*transaction);
                if (!receipt)
                    return core::fail(receipt.error());
                return toJson(*receipt);
            }
            if (method == Method::kGetAccount) {
                const auto address = addressParam(params);
                if (!address)
                    return core::fail(address.error());
                const auto account = api.getAccount(*address);
                if (!account)
                    return core::fail(account.error());
                return toJson(*account);
            }
            if (method == Method::kStatus) {
                const auto status = api.status();
                if (!status)
                    return core::fail(status.error());
                return toJson(*status);
            }
            return core::fail(core::ErrorCode::notFound, "unknown method '" + std::string(method) + "'");
        }
    }

    std::string JsonRpcHandler::handle(std::string_view request) {
        const auto parsed = nlohmann::json::parse(request, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object())
            return errorResponse(nullptr, -32700, "parse error");

        const auto id = parsed.value("id", nlohmann::json{});
        if (!parsed.contains("method") || !parsed["method"].is_string())
            return errorResponse(id, -32600, "invalid request");

        const auto params = parsed.value("params", nlohmann::json::array());
        const auto result = dispatch(*_api, parsed["method"].get<std::string>(), params);
        if (!result)
            return errorResponse(id, -32000, result.error().describe());

        const nlohmann::json response{
            {"jsonrpc", "2.0"},
            {"id", id},
            {"result", *result},
        };
        return response.dump();
    }

}

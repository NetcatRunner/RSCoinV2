#include "rpc/jsonrpc/Codec.hpp"

#include <nlohmann/json.hpp>

#include "core/Hex.hpp"
#include "primitives/Codec.hpp"

namespace RSCoin::Rpc {

    namespace {
        auto badParams(const char* expected) {
            return core::fail(core::ErrorCode::protocol, std::string("expected params: [") + expected + "]");
        }

        auto malformed(const char* what) {
            return core::fail(core::ErrorCode::protocol, std::string("malformed ") + what + " result");
        }

        core::Result<std::string> singleString(const nlohmann::json& params, const char* expected) {
            if (!params.is_array() || params.size() != 1 || !params[0].is_string())
                return badParams(expected);
            return params[0].get<std::string>();
        }
    }

    nlohmann::json paramsOf(const Primitives::Transaction& transaction) {
        return nlohmann::json::array({core::toHex(Primitives::encode(transaction))});
    }

    nlohmann::json paramsOf(const core::Address& address) {
        return nlohmann::json::array({core::toHex(address)});
    }

    core::Result<Primitives::Transaction> transactionParam(const nlohmann::json& params) {
        const auto hex = singleString(params, "rawTransactionHex");
        if (!hex)
            return core::fail(hex.error());
        const auto raw = core::fromHex(*hex);
        if (!raw)
            return core::fail(raw.error(), "raw transaction");
        auto transaction = Primitives::decodeTransaction(*raw);
        if (!transaction)
            return core::fail(transaction.error(), "raw transaction");
        return transaction;
    }

    core::Result<core::Address> addressParam(const nlohmann::json& params) {
        const auto hex = singleString(params, "addressHex");
        if (!hex)
            return core::fail(hex.error());
        const auto address = core::fixedFromHex<20>(*hex);
        if (!address)
            return core::fail(address.error(), "address");
        return address;
    }

    nlohmann::json toJson(const SendReceipt& receipt) {
        return {
            {"transactionHash", core::toHex(receipt.transactionHash)},
            {"outcome", receipt.outcome},
        };
    }

    nlohmann::json toJson(const AccountInfo& account) {
        return {
            {"balance", account.balance.units},
            {"nonce", account.nonce},
        };
    }

    nlohmann::json toJson(const NodeStatus& status) {
        return {
            {"height", status.height},
            {"headHash", core::toHex(status.headHash)},
            {"peers", status.peers},
            {"mempool", status.mempoolSize},
        };
    }

    core::Result<SendReceipt> receiptFromJson(const nlohmann::json& result) {
        if (!result.is_object() || !result.contains("transactionHash") || !result.contains("outcome"))
            return malformed("send-transaction");
        const auto hash = core::fixedFromHex<32>(result["transactionHash"].get<std::string>());
        if (!hash)
            return malformed("send-transaction");
        return SendReceipt{*hash, result["outcome"].get<std::string>()};
    }

    core::Result<AccountInfo> accountFromJson(const nlohmann::json& result) {
        if (!result.is_object() || !result.contains("balance") || !result.contains("nonce"))
            return malformed("get-account");
        return AccountInfo{core::Amount{result["balance"].get<std::uint64_t>()}, result["nonce"].get<std::uint64_t>()};
    }

    core::Result<NodeStatus> statusFromJson(const nlohmann::json& result) {
        if (!result.is_object() || !result.contains("height") || !result.contains("headHash") || !result.contains("peers") || !result.contains("mempool"))
            return malformed("status");
        const auto headHash = core::fixedFromHex<32>(result["headHash"].get<std::string>());
        if (!headHash)
            return malformed("status");
        return NodeStatus{result["height"].get<std::uint64_t>(), *headHash, result["peers"].get<std::size_t>(), result["mempool"].get<std::size_t>()};
    }

}

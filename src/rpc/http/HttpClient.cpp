#include "rpc/http/HttpRpcClient.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "core/Hex.hpp"
#include "primitives/Codec.hpp"
#include "rpc/api/Envelope.hpp"

namespace RSCoin::Rpc {

    namespace {
        auto malformed(std::string_view method) {
            return core::fail(core::ErrorCode::protocol, "malformed " + std::string(method) + " result");
        }
    }

    HttpRpcClient::HttpRpcClient(const std::string& host, std::uint16_t port)
        : _client(std::make_unique<httplib::Client>(host, port)) {}

    HttpRpcClient::~HttpRpcClient() = default;

    core::Result<nlohmann::json> HttpRpcClient::call(std::string_view method, const nlohmann::json& params) {
        const std::string body = buildRequest(_nextId++, method, params);

        const auto response = _client->Post("/", body, "application/json");
        if (!response)
            return core::fail(core::ErrorCode::network, "rpc request failed: " + httplib::to_string(response.error()));
        if (response->status != 200)
            return core::fail(core::ErrorCode::protocol, "rpc server returned http status " + std::to_string(response->status));

        return parseResponse(response->body);
    }

    core::Result<SendReceipt> HttpRpcClient::sendTransaction(const Primitives::Transaction& transaction) {
        const std::string rawHex = core::toHex(Primitives::encode(transaction));
        const auto result = call("rscoin_sendRawTransaction", nlohmann::json::array({rawHex}));
        if (!result)
            return core::fail(result.error());
        if (!result->contains("transactionHash") || !result->contains("outcome"))
            return malformed("rscoin_sendRawTransaction");

        const auto hash = core::fixedFromHex<32>((*result)["transactionHash"].get<std::string>());
        if (!hash)
            return malformed("rscoin_sendRawTransaction");
        return SendReceipt{*hash, (*result)["outcome"].get<std::string>()};
    }

    core::Result<AccountInfo> HttpRpcClient::getAccount(const core::Address& address) {
        const auto result = call("rscoin_getAccount", nlohmann::json::array({core::toHex(address)}));
        if (!result)
            return core::fail(result.error());
        if (!result->contains("balance") || !result->contains("nonce"))
            return malformed("rscoin_getAccount");

        return AccountInfo{core::Amount{(*result)["balance"].get<std::uint64_t>()},
                           (*result)["nonce"].get<std::uint64_t>()};
    }

    core::Result<NodeStatus> HttpRpcClient::status() {
        const auto result = call("rscoin_status", nlohmann::json::array());
        if (!result)
            return core::fail(result.error());
        if (!result->contains("height") || !result->contains("headHash") || !result->contains("peers") || !result->contains("mempool"))
            return malformed("rscoin_status");

        const auto headHash = core::fixedFromHex<32>((*result)["headHash"].get<std::string>());
        if (!headHash)
            return malformed("rscoin_status");
        return NodeStatus{(*result)["height"].get<std::uint64_t>(), *headHash,
                          (*result)["peers"].get<std::size_t>(), (*result)["mempool"].get<std::size_t>()};
    }

}

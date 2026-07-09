#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

#include "rpc/IRpcClient.hpp"

namespace httplib { class Client; }

namespace RSCoin::Rpc {

    // Variante http : POST / avec le corps JSON (convention Ethereum).
    class HttpRpcClient : public IRpcClient {
    public:
        HttpRpcClient(const std::string& host, std::uint16_t port);
        ~HttpRpcClient() override;

        core::Result<SendReceipt> sendTransaction(const Primitives::Transaction& transaction) override;
        core::Result<AccountInfo> getAccount(const core::Address& address) override;
        core::Result<NodeStatus> status() override;

    private:
        core::Result<nlohmann::json> call(std::string_view method, const nlohmann::json& params);

        std::unique_ptr<httplib::Client> _client;
        std::uint64_t _nextId{1};
    };

}

#include "wallet/web/WebUi.hpp"

#include <thread>

#include <RST/time/FrameTimer.hpp>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "core/Hex.hpp"
#include "log/Logger.hpp"
#include "wallet/web/Page.hpp"

namespace RSCoin::Wallet {

    static constexpr const char* kListenAddress = "127.0.0.1";

    WebUi::WebUi(IWallet& wallet, Rpc::INodeApi& node, std::uint16_t port)
        : _wallet(wallet), _node(node), _port(port), _server(std::make_unique<httplib::Server>()) {
        registerRoutes();
    }

    WebUi::~WebUi() = default;

    void WebUi::registerRoutes() {
        _server->Get("/", [](const httplib::Request&, httplib::Response& response) {
            response.set_content(std::string(kWalletPage), "text/html");
        });

        _server->Get("/api/wallet", [this](const httplib::Request&, httplib::Response& response) {
            std::lock_guard lock(_mutex);
            response.set_content(walletSnapshot(), "application/json");
        });

        _server->Post("/api/accounts", [this](const httplib::Request&, httplib::Response& response) {
            std::lock_guard lock(_mutex);
            int status = 200;
            response.set_content(createAccount(status), "application/json");
            response.status = status;
        });

        _server->Post("/api/send", [this](const httplib::Request& request, httplib::Response& response) {
            std::lock_guard lock(_mutex);
            int status = 200;
            response.set_content(sendValue(request.body, status), "application/json");
            response.status = status;
        });
    }

    std::string WebUi::walletSnapshot() {
        nlohmann::json payload{{"accounts", nlohmann::json::array()}, {"node", nullptr}};

        const auto status = _node.status();
        if (status)
            payload["node"] = {{"height", status->height}, {"peers", status->peers}, {"mempool", status->mempoolSize}};
        else
            payload["nodeError"] = status.error().describe();

        const auto addresses = _wallet.accounts();
        if (addresses) {
            for (const auto& address : *addresses) {
                nlohmann::json entry{{"address", core::toHex(address)}};
                if (status) {
                    if (const auto account = _node.getAccount(address)) {
                        entry["balance"] = account->balance.units;
                        entry["nonce"] = account->nonce;
                    }
                }
                payload["accounts"].push_back(std::move(entry));
            }
        }
        return payload.dump();
    }

    std::string WebUi::createAccount(int& status) {
        const auto address = _wallet.createAccount();
        if (!address) {
            status = 500;
            return nlohmann::json{{"error", address.error().describe()}}.dump();
        }
        RSCoin_INFO("new account: {}", core::toHex(*address));
        return nlohmann::json{{"address", core::toHex(*address)}}.dump();
    }

    std::string WebUi::sendValue(const std::string& body, int& status) {
        const auto fail = [&status](int code, const std::string& message) {
            status = code;
            return nlohmann::json{{"error", message}}.dump();
        };

        const auto request = nlohmann::json::parse(body, nullptr, false);
        if (request.is_discarded() || !request.contains("from") || !request.contains("to") || !request.contains("value"))
            return fail(400, "expected JSON body {from, to, value}");

        const auto from = core::fixedFromHex<20>(request["from"].get<std::string>());
        const auto to = core::fixedFromHex<20>(request["to"].get<std::string>());
        if (!from || !to || !request["value"].is_number_unsigned())
            return fail(400, "invalid address or amount");

        const auto account = _node.getAccount(*from);
        if (!account)
            return fail(502, account.error().describe());

        TransactionRequest transactionRequest;
        transactionRequest.from = *from;
        transactionRequest.to = *to;
        transactionRequest.value = core::Amount{request["value"].get<std::uint64_t>()};
        transactionRequest.nonce = account->nonce;

        auto transaction = _wallet.signTransaction(transactionRequest);
        if (!transaction)
            return fail(500, transaction.error().describe());

        const auto receipt = _node.sendTransaction(*transaction);
        if (!receipt)
            return fail(502, receipt.error().describe());

        RSCoin_INFO("transaction {} → {}", core::toHex(receipt->transactionHash), receipt->outcome);
        return nlohmann::json{{"transactionHash", core::toHex(receipt->transactionHash)}, {"outcome", receipt->outcome}}.dump();
    }

    core::Result<void> WebUi::run(std::stop_token stop) {
        if (!_server->bind_to_port(kListenAddress, _port))
            return core::fail(core::ErrorCode::network, "cannot bind wallet ui to " + std::string(kListenAddress) + ":" + std::to_string(_port));

        std::thread serving([this] { _server->listen_after_bind(); });
        RSCoin_INFO("wallet ui: http://{}:{}", kListenAddress, _port);

        auto frameTimer = RST::Time::FrameTimer(10);
        while (!stop.stop_requested())
            frameTimer.tick();

        _server->stop();
        serving.join();
        RSCoin_INFO("wallet ui stopped");
        return {};
    }

}

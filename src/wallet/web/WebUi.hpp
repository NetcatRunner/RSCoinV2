#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "rpc/INodeApi.hpp"
#include "wallet/IWallet.hpp"
#include "wallet/IWalletUi.hpp"

namespace httplib { class Server; }

namespace RSCoin::Wallet {

    class WebUi : public IWalletUi {
    public:
        WebUi(IWallet& wallet, Rpc::INodeApi& node, std::uint16_t port);
        ~WebUi() override;

        core::Result<void> run(std::stop_token stop) override;

    private:
        void registerRoutes();
        std::string walletSnapshot();
        std::string createAccount(int& status);
        std::string sendValue(const std::string& body, int& status);

        IWallet& _wallet;
        Rpc::INodeApi& _node;
        std::uint16_t _port;

        std::mutex _mutex;
        std::unique_ptr<httplib::Server> _server;
    };

}

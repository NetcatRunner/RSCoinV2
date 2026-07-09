#pragma once

#include <string>

#include "rpc/INodeApi.hpp"
#include "wallet/IWallet.hpp"
#include "wallet/IWalletUi.hpp"

namespace RSCoin::Wallet {

    struct Command {
        bool createAccount{};
        bool list{};
        bool send{};
        std::string balance;
        std::string from;
        std::string to;
        std::string value;

        bool empty() const noexcept { return !createAccount && !list && !send && balance.empty(); }
    };

    class TerminalUi : public IWalletUi {
    public:
        TerminalUi(IWallet& wallet, Rpc::INodeApi& node, Command command)
            : _wallet(wallet), _node(node), _command(std::move(command)) {}

        core::Result<void> run(std::stop_token stop) override;

    private:
        core::Result<void> createAccount();
        core::Result<void> listAccounts();
        core::Result<void> showBalance(const std::string& addressHex);
        core::Result<void> sendValue();

        core::Result<core::Address> resolveSender() const;

        IWallet& _wallet;
        Rpc::INodeApi& _node;
        Command _command;
    };

}

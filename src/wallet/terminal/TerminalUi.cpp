#include "wallet/terminal/TerminalUi.hpp"

#include <charconv>

#include "core/Hex.hpp"
#include "log/Logger.hpp"

namespace RSCoin::Wallet {

    core::Result<void> TerminalUi::run(std::stop_token) {
        if (_command.createAccount)
            return createAccount();
        if (_command.list)
            return listAccounts();
        if (!_command.balance.empty())
            return showBalance(_command.balance);
        if (_command.send)
            return sendValue();

        return core::fail(core::ErrorCode::config, "no command (use --new, --list, --balance or --send; see --help)");
    }

    core::Result<void> TerminalUi::createAccount() {
        auto address = _wallet.createAccount();
        if (!address)
            return core::fail(address.error());
        RSCoin_INFO("new account: {}", core::toHex(*address));
        return {};
    }

    core::Result<void> TerminalUi::listAccounts() {
        auto addresses = _wallet.accounts();
        if (!addresses)
            return core::fail(addresses.error());
        if (addresses->empty()) {
            RSCoin_INFO("no accounts (create one with --new)");
            return {};
        }
        for (const auto& address : *addresses)
            RSCoin_INFO("account: {}", core::toHex(address));
        return {};
    }

    core::Result<void> TerminalUi::showBalance(const std::string& addressHex) {
        const auto address = core::fixedFromHex<20>(addressHex);
        if (!address)
            return core::fail(address.error(), "--balance");

        const auto account = _node.getAccount(*address);
        if (!account)
            return core::fail(account.error());
        RSCoin_INFO("{}: balance {}, nonce {}", addressHex, account->balance.units, account->nonce);
        return {};
    }

    core::Result<core::Address> TerminalUi::resolveSender() const {
        if (!_command.from.empty())
            return core::fixedFromHex<20>(_command.from);

        auto addresses = _wallet.accounts();
        if (!addresses)
            return core::fail(addresses.error());
        if (addresses->size() != 1)
            return core::fail(core::ErrorCode::config, "--from is required when the keystore holds several accounts");
        return addresses->front();
    }

    core::Result<void> TerminalUi::sendValue() {
        const auto from = resolveSender();
        if (!from)
            return core::fail(from.error());
        const auto to = core::fixedFromHex<20>(_command.to);
        if (!to)
            return core::fail(to.error(), "--to");

        std::uint64_t value{};
        const auto [ptr, ec] = std::from_chars(_command.value.data(), _command.value.data() + _command.value.size(), value);
        if (ec != std::errc{} || ptr != _command.value.data() + _command.value.size())
            return core::fail(core::ErrorCode::config, "invalid amount '" + _command.value + "'");

        const auto account = _node.getAccount(*from);
        if (!account)
            return core::fail(account.error());

        TransactionRequest request;
        request.from = *from;
        request.to = *to;
        request.value = core::Amount{value};
        request.nonce = account->nonce;

        auto transaction = _wallet.signTransaction(request);
        if (!transaction)
            return core::fail(transaction.error());

        const auto receipt = _node.sendTransaction(*transaction);
        if (!receipt)
            return core::fail(receipt.error());

        RSCoin_INFO("transaction {} → {}", core::toHex(receipt->transactionHash), receipt->outcome);
        return {};
    }

}

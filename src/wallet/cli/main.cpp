#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

#include <RST/parser/ArgParser.hpp>

#include "chain/ChainConfig.hpp"
#include "config/Store.hpp"
#include "core/Hex.hpp"
#include "crypto/Factory.hpp"
#include "log/Logger.hpp"
#include "rpc/Factory.hpp"
#include "storage/Factory.hpp"
#include "wallet/Factory.hpp"
#include "wallet/WalletConfig.hpp"

namespace {

    using namespace RSCoin;

    constexpr const char* kDefaultNetworkPath = "config/networks/mainnet.json";
    constexpr const char* kDefaultConfigPath = "config/node.json";
    constexpr const char* kProgramName = "rscoin-wallet";

    struct Context {
        std::unique_ptr<Crypto::ICryptoProvider> crypto;
        std::unique_ptr<Storage::IKeyValueStore> keystore;
        std::unique_ptr<Wallet::IWallet> wallet;
        Rpc::RpcConfig rpc;
        std::uint64_t chainId{};
    };

    core::Result<Context> buildContext(const Config::Store& network, const Config::Store& node) {
        auto chainConfig = network.get<Chain::ChainConfig>();
        if (!chainConfig)
            return core::fail(chainConfig.error());

        auto cryptoConfig = network.get<Crypto::CryptoConfig>();
        if (!cryptoConfig)
            return core::fail(cryptoConfig.error());
        auto crypto = Crypto::makeProvider(*cryptoConfig);
        if (!crypto)
            return core::fail(crypto.error());

        auto walletConfig = node.get<Wallet::WalletConfig>();
        if (!walletConfig)
            return core::fail(walletConfig.error());
        auto keystore = Storage::makeStore(Storage::StorageConfig{"file", walletConfig->keystoreDirectory});
        if (!keystore)
            return core::fail(keystore.error(), "opening keystore");

        auto rpcConfig = node.get<Rpc::RpcConfig>();
        if (!rpcConfig)
            return core::fail(rpcConfig.error());

        auto wallet = Wallet::makeWallet(Wallet::WalletDeps{**crypto, **keystore, chainConfig->id});
        if (!wallet)
            return core::fail(wallet.error());

        return Context{std::move(*crypto), std::move(*keystore), std::move(*wallet), std::move(*rpcConfig), chainConfig->id};
    }

    core::Result<std::uint64_t> parseAmount(const std::string& text) {
        std::uint64_t value{};
        const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec != std::errc{} || ptr != text.data() + text.size())
            return core::fail(core::ErrorCode::config, "invalid amount '" + text + "'");
        return value;
    }

    core::Result<void> commandNew(Context& context) {
        auto address = context.wallet->createAccount();
        if (!address)
            return core::fail(address.error());
        RSCoin_INFO("new account: {}", core::toHex(*address));
        return {};
    }

    core::Result<void> commandList(const Context& context) {
        auto addresses = context.wallet->accounts();
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

    core::Result<void> commandBalance(const Context& context, const std::string& addressHex) {
        const auto address = core::fixedFromHex<20>(addressHex);
        if (!address)
            return core::fail(address.error(), "--balance");

        auto client = Rpc::makeRpcClient(context.rpc);
        if (!client)
            return core::fail(client.error());
        const auto account = (*client)->getAccount(*address);
        if (!account)
            return core::fail(account.error());
        RSCoin_INFO("{}: balance {}, nonce {}", addressHex, account->balance.units, account->nonce);
        return {};
    }

    core::Result<core::Address> resolveSender(const Context& context, const std::string& fromHex) {
        if (!fromHex.empty())
            return core::fixedFromHex<20>(fromHex);

        auto addresses = context.wallet->accounts();
        if (!addresses)
            return core::fail(addresses.error());
        if (addresses->size() != 1)
            return core::fail(core::ErrorCode::config, "--from is required when the keystore holds several accounts");
        return addresses->front();
    }

    core::Result<void> commandSend(Context& context, const std::string& fromHex, const std::string& toHex, const std::string& valueText) {
        const auto from = resolveSender(context, fromHex);
        if (!from)
            return core::fail(from.error());
        const auto to = core::fixedFromHex<20>(toHex);
        if (!to)
            return core::fail(to.error(), "--to");
        const auto value = parseAmount(valueText);
        if (!value)
            return core::fail(value.error());

        auto client = Rpc::makeRpcClient(context.rpc);
        if (!client)
            return core::fail(client.error());

        const auto account = (*client)->getAccount(*from);
        if (!account)
            return core::fail(account.error());

        Wallet::TransactionRequest request;
        request.from = *from;
        request.to = *to;
        request.value = core::Amount{*value};
        request.nonce = account->nonce;

        auto transaction = context.wallet->signTransaction(request);
        if (!transaction)
            return core::fail(transaction.error());

        const auto receipt = (*client)->sendTransaction(*transaction);
        if (!receipt)
            return core::fail(receipt.error());

        RSCoin_INFO("transaction {} → {}", core::toHex(receipt->transactionHash), receipt->outcome);
        return {};
    }

    void initParser(RST::Parser::ArgParser& parser) {
        parser.addFlag({"--help", "-h"}, "Display this help message");
        parser.addOption({"--network", "-n"}, "Path to the network definition file (shared by all peers)", kDefaultNetworkPath);
        parser.addOption({"--config", "-c"}, "Path to the personal node configuration file", kDefaultConfigPath);
        parser.addFlag({"--new"}, "Create a new account");
        parser.addFlag({"--list"}, "List the keystore accounts");
        parser.addOption({"--balance"}, "Show balance and nonce of an address", "");
        parser.addFlag({"--send"}, "Send value (requires --to and --value)");
        parser.addOption({"--from"}, "Sender address (optional if the keystore holds one account)", "");
        parser.addOption({"--to"}, "Recipient address", "");
        parser.addOption({"--value"}, "Amount to send", "");
    }

    core::Result<void> runCommand(RST::Parser::ArgParser& parser) {
        auto network = Config::Store::load(parser.getOption<std::string>("--network"));
        if (!network)
            return core::fail(network.error());
        auto node = Config::Store::load(parser.getOption<std::string>("--config"));
        if (!node)
            return core::fail(node.error());
        auto context = buildContext(*network, *node);
        if (!context)
            return core::fail(context.error());

        if (parser.isSet("--new"))
            return commandNew(*context);
        if (parser.isSet("--list"))
            return commandList(*context);
        if (parser.isSet("--balance"))
            return commandBalance(*context, parser.getOption<std::string>("--balance"));
        if (parser.isSet("--send"))
            return commandSend(*context, parser.getOption<std::string>("--from"), parser.getOption<std::string>("--to"), parser.getOption<std::string>("--value"));

        return core::fail(core::ErrorCode::config, "no command (use --new, --list, --balance or --send; see --help)");
    }

}

int main(int argc, const char* argv[]) {
    RSCoin::Log::Logger::Init();

    RST::Parser::ArgParser parser;
    initParser(parser);
    try {
        parser.parse(argc, argv);
    } catch (const RST::Parser::HelpRequested&) {
        parser.printHelp(kProgramName);
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        RSCoin_FATAL("invalid arguments: {}", error.what());
        return EXIT_FAILURE;
    }

    if (const auto result = runCommand(parser); !result) {
        RSCoin_FATAL("{}", result.error().describe());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

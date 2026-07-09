#include <cstdlib>
#include <filesystem>
#include <string>

#include <RST/parser/ArgParser.hpp>

#include "chain/ChainConfig.hpp"
#include "config/Store.hpp"
#include "crypto/Factory.hpp"
#include "log/Logger.hpp"
#include "rpc/Factory.hpp"
#include "storage/Factory.hpp"
#include "utils/SignalHandler.hpp"
#include "wallet/Factory.hpp"

namespace {

    using namespace RSCoin;

    constexpr const char* kDefaultNetworkPath = "config/networks/mainnet.json";
    constexpr const char* kDefaultConfigPath = "config/node.json";
    constexpr const char* kProgramName = "rscoin-wallet";

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

    Wallet::Command parseCommand(const RST::Parser::ArgParser& parser) {
        Wallet::Command command;
        command.createAccount = parser.isSet("--new");
        command.list = parser.isSet("--list");
        command.send = parser.isSet("--send");
        command.balance = parser.getOption<std::string>("--balance");
        command.from = parser.getOption<std::string>("--from");
        command.to = parser.getOption<std::string>("--to");
        command.value = parser.getOption<std::string>("--value");
        return command;
    }

    core::Result<void> runWallet(const RST::Parser::ArgParser& parser) {
        auto network = Config::Store::load(parser.getOption<std::string>("--network"));
        if (!network)
            return core::fail(network.error());
        auto node = Config::Store::load(parser.getOption<std::string>("--config"));
        if (!node)
            return core::fail(node.error());

        auto chainConfig = network->get<Chain::ChainConfig>();
        if (!chainConfig)
            return core::fail(chainConfig.error());

        auto crypto = network->get<Crypto::CryptoConfig>().and_then(Crypto::makeProvider);
        if (!crypto)
            return core::fail(crypto.error());

        auto walletConfig = node->get<Wallet::WalletConfig>();
        if (!walletConfig)
            return core::fail(walletConfig.error());
        auto keystore = Storage::makeStore(Storage::StorageConfig{"file", walletConfig->keystoreDirectory});
        if (!keystore)
            return core::fail(keystore.error(), "opening keystore");

        auto wallet = Wallet::makeWallet(Wallet::WalletDeps{**crypto, **keystore, chainConfig->id});
        if (!wallet)
            return core::fail(wallet.error());

        auto client = node->get<Rpc::RpcConfig>().and_then(Rpc::makeRpcClient);
        if (!client)
            return core::fail(client.error());

        auto ui = Wallet::makeWalletUi(Wallet::WalletUiDeps{**wallet, **client}, *walletConfig, parseCommand(parser));
        if (!ui)
            return core::fail(ui.error());
        return (*ui)->run(Utils::stopSource().get_token());
    }

}

int main(int argc, const char* argv[]) {
    RSCoin::Log::Logger::Init();
    RSCoin::Utils::setupHandlers();

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

    if (const auto result = runWallet(parser); !result) {
        RSCoin_FATAL("{}", result.error().describe());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#include <cstdlib>
#include <filesystem>
#include <utility>

#include <RST/parser/ArgParser.hpp>

#include "chain/Factory.hpp"
#include "chain/Genesis.hpp"
#include "config/NodeConfig.hpp"
#include "consensus/EngineFactory.hpp"
#include "crypto/Factory.hpp"
#include "log/Logger.hpp"
#include "mempool/Factory.hpp"
#include "network/Factory.hpp"
#include "node/Node.hpp"
#include "protocol/Factory.hpp"
#include "state/Factory.hpp"
#include "storage/Factory.hpp"
#include "utils/SignalHandler.hpp"

namespace {

    constexpr const char* kDefaultConfigPath = "config/node.pow.json";
    constexpr const char* kProgramName = "RSCoin2";

    void initParser(RST::Parser::ArgParser& parser) {
        parser.addFlag({"--help", "-h"}, "Display this help message");
        parser.addOption({"--config", "-c"}, "Path to the node configuration file", kDefaultConfigPath);
    }

    RSCoin::core::Result<std::filesystem::path> parseArguments(int argc, const char** argv) {
        RST::Parser::ArgParser parser;
        initParser(parser);

        try {
            parser.parse(argc, argv);
        } catch (const RST::Parser::HelpRequested&) {
            parser.printHelp(kProgramName);
            return std::filesystem::path{};
        } catch (const std::exception& error) {
            return RSCoin::core::fail(RSCoin::core::ErrorCode::config, std::string("invalid arguments: ") + error.what());
        }

        return std::filesystem::path{parser.getOption<std::string>("--config")};
    }

    RSCoin::core::Result<RSCoin::Node::Modules> buildModules(const RSCoin::Config::NodeConfig& config) {
        using namespace RSCoin;

        auto crypto = Crypto::makeProvider(config.crypto);
        if (!crypto)
            return core::fail(crypto.error());

        auto storage = Storage::makeStore(config.storage);
        if (!storage)
            return core::fail(storage.error());

        auto state = State::makeStateMachine(**crypto, config);
        if (!state)
            return core::fail(state.error());

        const Primitives::Block genesis = Chain::buildGenesis(config.genesis, (*state)->stateRoot());
        auto chain = Chain::makeBlockchain(**storage, (*crypto)->hasher(), genesis);
        if (!chain)
            return core::fail(chain.error());

        auto consensus = Consensus::makeEngine(config.consensus, **crypto);
        if (!consensus)
            return core::fail(consensus.error());

        auto manager = Chain::makeChainManager(Chain::ChainManagerDeps{**chain, **consensus, (*crypto)->hasher()}, std::move(*state));
        if (!manager)
            return core::fail(manager.error());

        auto network = Network::makeNetwork(config.network);
        if (!network)
            return core::fail(network.error());

        auto protocol = Protocol::makeProtocol(config, **network, Protocol::ChainServices{**chain, **manager, (*crypto)->hasher()});
        if (!protocol)
            return core::fail(protocol.error());

        auto mempool = Mempool::makeMempool();
        if (!mempool)
            return core::fail(mempool.error());

        return Node::Modules{
            .crypto = std::move(*crypto),
            .storage = std::move(*storage),
            .chain = std::move(*chain),
            .consensus = std::move(*consensus),
            .manager = std::move(*manager),
            .network = std::move(*network),
            .protocol = std::move(*protocol),
            .mempool = std::move(*mempool),
        };
    }

    RSCoin::core::Result<void> runNode(const std::filesystem::path& configPath) {

        auto config = RSCoin::Config::loadFromFile(configPath);
        if (!config)
            return RSCoin::core::fail(config.error(), "loading '" + configPath.string() + "'");

        auto modules = buildModules(*config);
        if (!modules)
            return RSCoin::core::fail(modules.error());

        RSCoin_INFO("starting node (chainId {}, consensus '{}', protocol '{}')", config->chainId, config->consensus.engine, config->protocol.name);

        RSCoin::Node::Node node{std::move(*modules), std::move(*config)};
        return node.run(RSCoin::Utils::stopSource().get_token());
    }
}

int main(int argc, const char* argv[]) {
    RSCoin::Log::Logger::Init();
    RSCoin::Utils::setupHandlers();

    const auto configPath = parseArguments(argc, argv);
    if (!configPath) {
        RSCoin_FATAL("{}", configPath.error().describe());
        return EXIT_FAILURE;
    }
    if (configPath->empty())
        return EXIT_SUCCESS;

    const auto result = runNode(*configPath);
    if (!result) {
        RSCoin_FATAL("{}", result.error().describe());
        return EXIT_FAILURE;
    }

    RSCoin_INFO("node shut down cleanly");
    return EXIT_SUCCESS;
}

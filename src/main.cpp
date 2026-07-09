#include <cstdlib>
#include <filesystem>
#include <utility>

#include <RST/parser/ArgParser.hpp>

#include "chain/ChainConfig.hpp"
#include "chain/Factory.hpp"
#include "chain/genesis/Genesis.hpp"
#include "config/Store.hpp"
#include "consensus/Factory.hpp"
#include "crypto/Factory.hpp"
#include "log/Logger.hpp"
#include "mempool/Factory.hpp"
#include "mining/Factory.hpp"
#include "network/Factory.hpp"
#include "node/Node.hpp"
#include "protocol/Factory.hpp"
#include "rpc/Factory.hpp"
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

    RSCoin::core::Result<RSCoin::Node::Modules> buildModules(const RSCoin::Config::Store& store) {
        using namespace RSCoin;

        auto chainConfig = store.get<Chain::ChainConfig>();
        if (!chainConfig)
            return core::fail(chainConfig.error());

        auto cryptoConfig = store.get<Crypto::CryptoConfig>();
        if (!cryptoConfig)
            return core::fail(cryptoConfig.error());
        auto crypto = Crypto::makeProvider(*cryptoConfig);
        if (!crypto)
            return core::fail(crypto.error());

        auto storageConfig = store.get<Storage::StorageConfig>();
        if (!storageConfig)
            return core::fail(storageConfig.error());
        auto storage = Storage::makeStore(*storageConfig);
        if (!storage)
            return core::fail(storage.error());

        auto genesisConfig = store.get<Chain::GenesisConfig>();
        if (!genesisConfig)
            return core::fail(genesisConfig.error());
        auto stateConfig = store.get<State::StateConfig>();
        if (!stateConfig)
            return core::fail(stateConfig.error());

        auto state = State::makeStateMachine(**crypto, chainConfig->id, *stateConfig, *genesisConfig);
        if (!state)
            return core::fail(state.error());

        const Primitives::Block genesis = Chain::buildGenesis(*genesisConfig, (*state)->stateRoot());
        auto chain = Chain::makeBlockchain(**storage, (*crypto)->hasher(), genesis);
        if (!chain)
            return core::fail(chain.error());

        auto consensusConfig = store.get<Consensus::ConsensusConfig>();
        if (!consensusConfig)
            return core::fail(consensusConfig.error());
        auto consensus = Consensus::makeEngine(*consensusConfig, **crypto);
        if (!consensus)
            return core::fail(consensus.error());

        auto manager = Chain::makeChainManager(Chain::ChainManagerDeps{**chain, **consensus, (*crypto)->hasher()}, std::move(*state));
        if (!manager)
            return core::fail(manager.error());

        auto mempool = Mempool::makeMempool(**manager);
        if (!mempool)
            return core::fail(mempool.error());

        auto networkConfig = store.get<Network::NetworkConfig>();
        if (!networkConfig)
            return core::fail(networkConfig.error());
        auto network = Network::makeNetwork(*networkConfig);
        if (!network)
            return core::fail(network.error());

        auto protocolConfig = store.get<Protocol::ProtocolConfig>();
        if (!protocolConfig)
            return core::fail(protocolConfig.error());
        auto protocol = Protocol::makeProtocol(*protocolConfig, chainConfig->id, **network, Protocol::NodeServices{**chain, **manager, (*crypto)->hasher(), **mempool});
        if (!protocol)
            return core::fail(protocol.error());

        auto rpcConfig = store.get<Rpc::RpcConfig>();
        if (!rpcConfig)
            return core::fail(rpcConfig.error());

        std::unique_ptr<Rpc::IRpcServer> rpc;
        if (rpcConfig->enabled) {
            auto made = Rpc::makeRpcServer(*rpcConfig, Rpc::NodeServices{**chain, **manager, **mempool, **network, (*crypto)->hasher()});
            if (!made)
                return core::fail(made.error());
            rpc = std::move(*made);
        }

        auto miningConfig = store.get<Mining::MiningConfig>();
        if (!miningConfig)
            return core::fail(miningConfig.error());

        std::unique_ptr<Mining::IMiner> miner;
        if (miningConfig->enabled) {
            auto blockConfig = store.get<Chain::BlockConfig>();
            if (!blockConfig)
                return core::fail(blockConfig.error());

            auto made = Mining::makeMiner(Mining::MinerDeps{**chain, **consensus, **manager, **mempool}, std::move(*miningConfig), std::move(*blockConfig));
            if (!made)
                return core::fail(made.error());
            miner = std::move(*made);
        }

        RSCoin_INFO("starting node (chainId {}, consensus '{}', protocol '{}')", chainConfig->id, consensusConfig->engine, protocolConfig->name);

        return Node::Modules{
            .crypto = std::move(*crypto),
            .storage = std::move(*storage),
            .chain = std::move(*chain),
            .consensus = std::move(*consensus),
            .mempool = std::move(*mempool),
            .manager = std::move(*manager),
            .network = std::move(*network),
            .protocol = std::move(*protocol),
            .rpc = std::move(rpc),
            .miner = std::move(miner),
        };
    }

    RSCoin::core::Result<void> runNode(const std::filesystem::path& configPath) {
        auto store = RSCoin::Config::Store::load(configPath);
        if (!store)
            return RSCoin::core::fail(store.error(), "loading '" + configPath.string() + "'");

        auto modules = buildModules(*store);
        if (!modules)
            return RSCoin::core::fail(modules.error());

        RSCoin::Node::Node node{std::move(*modules)};
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

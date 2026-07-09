#include <cstdlib>
#include <filesystem>

#include <RST/parser/ArgParser.hpp>

#include "config/Store.hpp"
#include "log/Logger.hpp"
#include "node/Factory.hpp"
#include "utils/SignalHandler.hpp"

namespace {

    constexpr const char* kDefaultNetworkPath = "config/networks/mainnet.json";
    constexpr const char* kDefaultConfigPath = "config/node.json";
    constexpr const char* kProgramName = "RSCoin2";

    struct ConfigPaths {
        std::filesystem::path network;
        std::filesystem::path node;
    };

    void initParser(RST::Parser::ArgParser& parser) {
        parser.addFlag({"--help", "-h"}, "Display this help message");
        parser.addOption({"--network", "-n"}, "Path to the network definition file (shared by all peers)", kDefaultNetworkPath);
        parser.addOption({"--config", "-c"}, "Path to the personal node configuration file", kDefaultConfigPath);
    }

    RSCoin::core::Result<ConfigPaths> parseArguments(int argc, const char** argv) {
        RST::Parser::ArgParser parser;
        initParser(parser);

        try {
            parser.parse(argc, argv);
        } catch (const RST::Parser::HelpRequested&) {
            parser.printHelp(kProgramName);
            return ConfigPaths{};
        } catch (const std::exception& error) {
            return RSCoin::core::fail(RSCoin::core::ErrorCode::config, std::string("invalid arguments: ") + error.what());
        }

        return ConfigPaths{parser.getOption<std::string>("--network"), parser.getOption<std::string>("--config")};
    }

    RSCoin::core::Result<void> runNode(const ConfigPaths& paths) {
        auto network = RSCoin::Config::Store::load(paths.network);
        if (!network)
            return RSCoin::core::fail(network.error(), "loading network '" + paths.network.string() + "'");

        auto node = RSCoin::Config::Store::load(paths.node);
        if (!node)
            return RSCoin::core::fail(node.error(), "loading '" + paths.node.string() + "'");

        auto instance = RSCoin::Node::makeNode(*network, *node);
        if (!instance)
            return RSCoin::core::fail(instance.error());
        return instance->run(RSCoin::Utils::stopSource().get_token());
    }
}

int main(int argc, const char* argv[]) {
    RSCoin::Log::Logger::Init();
    RSCoin::Utils::setupHandlers();

    const auto paths = parseArguments(argc, argv);
    if (!paths) {
        RSCoin_FATAL("{}", paths.error().describe());
        return EXIT_FAILURE;
    }
    if (paths->network.empty())
        return EXIT_SUCCESS;

    const auto result = runNode(*paths);
    if (!result) {
        RSCoin_FATAL("{}", result.error().describe());
        return EXIT_FAILURE;
    }

    RSCoin_INFO("node shut down cleanly");
    return EXIT_SUCCESS;
}

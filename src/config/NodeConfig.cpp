#include "NodeConfig.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "core/Hex.hpp"

using json = nlohmann::json;


namespace RSCoin::Config {

    namespace {
        core::Address addressFromJson(const json& value) {
            auto address = core::fixedFromHex<20>(value.get<std::string>());
            if (!address)
                throw std::invalid_argument(address.error().message);
            return *address;
        }

        core::Bytes bytesFromJson(const json& value) {
            auto bytes = core::fromHex(value.get<std::string>());
            if (!bytes)
                throw std::invalid_argument(bytes.error().message);
            return *bytes;
        }
    }

    void from_json(const json& j, GenesisAccount& obj) {
        obj.address = addressFromJson(j.at("address"));
        obj.balance = core::Amount{j.at("balance").get<std::uint64_t>()};
    }

    void from_json(const json& j, GenesisConfig& obj) {
        j.at("timestamp").get_to(obj.timestamp);
        j.at("allocations").get_to(obj.allocations);
        obj.consensusSeal = bytesFromJson(j.at("consensusSeal"));
    }

    void from_json(const json& j, StateConfig& obj) {
        obj.blockReward = core::Amount{j.at("blockReward").get<std::uint64_t>()};
    }

    void from_json(const json& j, MiningConfig& obj) {
        j.at("enabled").get_to(obj.enabled);
        if (obj.enabled)
            obj.beneficiary = addressFromJson(j.at("beneficiary"));
    }

    void from_json(const json& j, ConsensusConfig& obj) {
        j.at("engine").get_to(obj.engine);
        j.at("parameters").get_to(obj.parameters);
    }

    void from_json(const json& j, BlockConfig& obj) {
        j.at("maxTransactions").get_to(obj.maxTransactions);
        j.at("maxExtraDataBytes").get_to(obj.maxExtraDataBytes);
        j.at("enabledExtensions").get_to(obj.enabledExtensions);
    }

    void from_json(const json& j, CryptoConfig& obj) {
        j.at("hasher").get_to(obj.hasher);
        j.at("signatureScheme").get_to(obj.signatureScheme);
    }

    void from_json(const json& j, NetworkConfig& obj) {
        j.at("transport").get_to(obj.transport);
        j.at("listenAddress").get_to(obj.listenAddress);
        j.at("port").get_to(obj.port);
        j.at("maxPeers").get_to(obj.maxPeers);
        j.at("maxMessageBytes").get_to(obj.maxMessageBytes);
        j.at("bootstrapPeers").get_to(obj.bootstrapPeers);
    }

    void from_json(const json& j, ProtocolConfig& obj) {
        j.at("name").get_to(obj.name);
        j.at("version").get_to(obj.version);
    }

    void from_json(const json& j, StorageConfig& obj) {
        j.at("backend").get_to(obj.backend);

        std::string dir;
        j.at("directory").get_to(dir);
        obj.directory = dir;
    }

    void from_json(const json& j, NodeConfig& obj) {
        j.at("chainId").get_to(obj.chainId);
        j.at("genesis").get_to(obj.genesis);
        j.at("block").get_to(obj.block);
        j.at("consensus").get_to(obj.consensus);
        j.at("crypto").get_to(obj.crypto);
        j.at("network").get_to(obj.network);
        j.at("protocol").get_to(obj.protocol);
        j.at("storage").get_to(obj.storage);
        j.at("state").get_to(obj.state);
        j.at("mining").get_to(obj.mining);
    }

    core::Result<NodeConfig> loadFromFile(const std::filesystem::path& path) {
        std::ifstream file(path);
        
        if (!file.is_open()) {
            return core::fail(core::ErrorCode::config, "Failed to open configuration file: " + path.string());
        }
        try {
            nlohmann::json j = nlohmann::json::parse(file);
            NodeConfig config = j.get<NodeConfig>();
            return config; 
        } catch (const nlohmann::json::parse_error& e) {
            return core::fail(core::ErrorCode::config, "JSON syntax error at byte " + std::to_string(e.byte) + ": " + e.what());
            
        } catch (const nlohmann::json::out_of_range& e) {
            return core::fail(core::ErrorCode::config, std::string("Missing key in configuration file: ") + e.what());
            
        } catch (const nlohmann::json::type_error& e) {
            return core::fail(core::ErrorCode::config, std::string("Type error in configuration file: ") + e.what());
            
        } catch (const std::exception& e) {
            return core::fail(core::ErrorCode::config,  std::string("Unexpected error while reading configuration: ") + e.what());
        }
    }
}

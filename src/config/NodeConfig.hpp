#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Config {

    struct GenesisAccount {
        core::Address address;
        core::Amount balance;
    };

    struct GenesisConfig {
        std::uint64_t timestamp{};
        std::vector<GenesisAccount> allocations;
        core::Bytes consensusSeal;
    };

    struct ConsensusConfig {
        std::string engine;
        std::map<std::string, std::string> parameters;
    };

    struct BlockConfig {
        std::size_t maxTransactions{};
        std::size_t maxExtraDataBytes{};
        std::vector<std::uint16_t> enabledExtensions;
    };

    struct CryptoConfig {
        std::string hasher;
        std::string signatureScheme;
    };

    struct NetworkConfig {
        std::string transport;
        std::string listenAddress;
        std::uint16_t port{};
        std::size_t maxPeers{};
        std::size_t maxMessageBytes{};
        std::vector<std::string> bootstrapPeers;
    };

    struct ProtocolConfig {
        std::string name;
        std::uint32_t version{};
    };

    struct StorageConfig {
        std::string backend;
        std::filesystem::path directory;
    };

    struct StateConfig {
        core::Amount blockReward{};
    };

    struct MiningConfig {
        bool enabled{};
        core::Address beneficiary;
    };

    struct NodeConfig {
        std::uint64_t chainId{};
        GenesisConfig genesis;
        BlockConfig block;
        ConsensusConfig consensus;
        CryptoConfig crypto;
        NetworkConfig network;
        ProtocolConfig protocol;
        StorageConfig storage;
        StateConfig state;
        MiningConfig mining;
    };

    core::Result<NodeConfig> loadFromFile(const std::filesystem::path& path);

}

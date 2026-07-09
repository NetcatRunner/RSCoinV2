#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Network {

    struct NetworkConfig {
        static constexpr const char* kSection = "network";

        std::string transport;
        std::string listenAddress;
        std::uint16_t port{};
        std::size_t maxPeers{};
        std::size_t maxMessageBytes{};
        std::vector<std::string> bootstrapPeers;

        static core::Result<NetworkConfig> from(const Config::Section& section);
    };

}

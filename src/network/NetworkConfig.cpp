#include "network/NetworkConfig.hpp"

namespace RSCoin::Network {

    core::Result<NetworkConfig> NetworkConfig::from(const Config::Section& section) {
        NetworkConfig config;
        Config::Reader reader(section);
        reader.read("transport", config.transport);
        reader.read("listenAddress", config.listenAddress);
        reader.read("port", config.port);
        reader.read("maxPeers", config.maxPeers);
        reader.read("maxMessageBytes", config.maxMessageBytes);
        reader.read("bootstrapPeers", config.bootstrapPeers);
        return reader.finish(std::move(config));
    }

}

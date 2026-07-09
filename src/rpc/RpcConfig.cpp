#include "rpc/RpcConfig.hpp"

namespace RSCoin::Rpc {

    core::Result<RpcConfig> RpcConfig::from(const Config::Section& section) {
        RpcConfig config;
        Config::Reader reader(section);
        reader.read("enabled", config.enabled);
        if (config.enabled) {
            reader.read("transport", config.transport);
            reader.read("listenAddress", config.listenAddress);
            reader.read("port", config.port);
        }
        return reader.finish(std::move(config));
    }

}

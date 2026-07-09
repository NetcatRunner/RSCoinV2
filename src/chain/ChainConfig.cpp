#include "chain/ChainConfig.hpp"

namespace RSCoin::Chain {

    core::Result<ChainConfig> ChainConfig::from(const Config::Section& section) {
        ChainConfig config;
        Config::Reader reader(section);
        reader.read("id", config.id);
        return reader.finish(std::move(config));
    }

}

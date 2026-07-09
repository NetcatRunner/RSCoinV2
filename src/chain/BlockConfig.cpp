#include "chain/BlockConfig.hpp"

namespace RSCoin::Chain {

    core::Result<BlockConfig> BlockConfig::from(const Config::Section& section) {
        BlockConfig config;
        Config::Reader reader(section);
        reader.read("maxTransactions", config.maxTransactions);
        reader.read("maxExtraDataBytes", config.maxExtraDataBytes);
        reader.read("enabledExtensions", config.enabledExtensions);
        return reader.finish(std::move(config));
    }

}

#include "state/StateConfig.hpp"

namespace RSCoin::State {

    core::Result<StateConfig> StateConfig::from(const Config::Section& section) {
        StateConfig config;
        Config::Reader reader(section);
        reader.read("blockReward", config.blockReward);
        return reader.finish(std::move(config));
    }

}

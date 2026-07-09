#include "consensus/ConsensusConfig.hpp"

namespace RSCoin::Consensus {

    core::Result<ConsensusConfig> ConsensusConfig::from(const Config::Section& section) {
        ConsensusConfig config;
        Config::Reader reader(section);
        reader.read("engine", config.engine);
        reader.read("parameters", config.parameters);
        return reader.finish(std::move(config));
    }

}

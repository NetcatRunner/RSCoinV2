#include "mining/MiningConfig.hpp"

namespace RSCoin::Mining {

    core::Result<MiningConfig> MiningConfig::from(const Config::Section& section) {
        MiningConfig config;
        Config::Reader reader(section);
        reader.read("enabled", config.enabled);
        if (config.enabled)
            reader.read("beneficiary", config.beneficiary);
        return reader.finish(std::move(config));
    }

}

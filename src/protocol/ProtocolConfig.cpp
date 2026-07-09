#include "protocol/ProtocolConfig.hpp"

namespace RSCoin::Protocol {

    core::Result<ProtocolConfig> ProtocolConfig::from(const Config::Section& section) {
        ProtocolConfig config;
        Config::Reader reader(section);
        reader.read("name", config.name);
        reader.read("version", config.version);
        return reader.finish(std::move(config));
    }

}

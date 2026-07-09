#include "storage/StorageConfig.hpp"

namespace RSCoin::Storage {

    core::Result<StorageConfig> StorageConfig::from(const Config::Section& section) {
        StorageConfig config;
        std::string directory;

        Config::Reader reader(section);
        reader.read("backend", config.backend);
        reader.read("directory", directory);
        config.directory = std::move(directory);
        return reader.finish(std::move(config));
    }

}

#include "storage/Factory.hpp"

#include "storage/FileStore.hpp"
#include "storage/MemoryStore.hpp"

namespace RSCoin::Storage {

    core::Result<std::unique_ptr<IKeyValueStore>> makeStore(const Config::StorageConfig& config) {
        if (config.backend == "memory")
            return std::make_unique<MemoryStore>();
        if (config.backend == "file") {
            auto store = std::make_unique<FileStore>(config.directory);
            if (auto initialized = store->init(); !initialized)
                return core::fail(initialized.error());
            return store;
        }
        return core::fail(core::ErrorCode::storage, "unknown storage backend: '" + config.backend + "'");
    }

}

#include "storage/Factory.hpp"

#include "storage/file/FileStore.hpp"
#include "storage/memory/MemoryStore.hpp"

namespace RSCoin::Storage {

    core::Result<std::unique_ptr<IKeyValueStore>> makeStore(const StorageConfig& config) {
        if (config.backend == "memory")
            return std::make_unique<MemoryStore>();

        if (config.backend == "file") {
            auto store = std::make_unique<FileStore>(config.directory);
            if (auto ready = store->init(); !ready)
                return core::fail(ready.error());
            return store;
        }
        return core::fail(core::ErrorCode::storage, "unknown storage backend: '" + config.backend + "'");
    }

}

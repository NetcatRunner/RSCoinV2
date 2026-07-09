#pragma once

#include <memory>

#include "storage/StorageConfig.hpp"
#include "core/Result.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Storage {

    core::Result<std::unique_ptr<IKeyValueStore>> makeStore(const StorageConfig& config);

}

#pragma once

#include <memory>

#include "config/NodeConfig.hpp"
#include "core/Result.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Storage {

    core::Result<std::unique_ptr<IKeyValueStore>> makeStore(const Config::StorageConfig& config);

}

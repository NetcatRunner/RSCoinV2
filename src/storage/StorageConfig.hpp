#pragma once

#include <filesystem>
#include <string>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Storage {

    struct StorageConfig {
        static constexpr const char* kSection = "storage";

        std::string backend;
        std::filesystem::path directory;

        static core::Result<StorageConfig> from(const Config::Section& section);
    };

}

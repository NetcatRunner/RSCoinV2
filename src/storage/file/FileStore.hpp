#pragma once

#include <filesystem>

#include "storage/memory/MemoryStore.hpp"

namespace RSCoin::Storage {

    class FileStore : public MemoryStore {
    public:
        FileStore(std::filesystem::path directory);
        // static core::Result<std::unique_ptr<FileStore>> open(std::filesystem::path directory);

        core::Result<void> init();

        core::Result<void> put(core::BytesView key, core::BytesView value) override;
        core::Result<void> erase(core::BytesView key) override;
        core::Result<void> apply(WriteBatch batch) override;
    private:
        // FileStore(std::filesystem::path file) : _file(std::move(file)) {}

        core::Result<void> load();
        core::Result<void> save() const;

        std::filesystem::path _file;
    };

}

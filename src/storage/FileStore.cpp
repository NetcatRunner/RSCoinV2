#include "storage/FileStore.hpp"

#include <cstdint>
#include <fstream>

namespace RSCoin::Storage {

    namespace {
        constexpr char kFileName[] = "store.dat";

        void writeSized(std::ofstream& out, const core::Bytes& bytes) {
            const auto size = static_cast<std::uint64_t>(bytes.size());
            out.write(reinterpret_cast<const char*>(&size), sizeof(size));
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        bool readSized(std::ifstream& in, core::Bytes& bytes) {
            std::uint64_t size = 0;
            if (!in.read(reinterpret_cast<char*>(&size), sizeof(size)))
                return false;
            bytes.resize(size);
            return static_cast<bool>(in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)));
        }
    }

    FileStore::FileStore(std::filesystem::path directory) 
        : _file(std::move(directory) / kFileName)
    {}

    core::Result<void> FileStore::init() {
        std::error_code errorCode;
        std::filesystem::create_directories(_file.parent_path(), errorCode);
        if (errorCode)
            return core::fail(core::ErrorCode::storage, "cannot create storage directory '" + _file.parent_path().string() + "': " + errorCode.message());

        return load();
    }

    core::Result<void> FileStore::load() {
        std::ifstream in(_file, std::ios::binary);
        if (!in)
            return {};

        std::uint64_t count = 0;
        if (!in.read(reinterpret_cast<char*>(&count), sizeof(count)))
            return core::fail(core::ErrorCode::storage, "corrupted store file '" + _file.string() + "'");

        for (std::uint64_t i = 0; i < count; ++i) {
            core::Bytes key, value;
            if (!readSized(in, key) || !readSized(in, value)) {
                return core::fail(core::ErrorCode::storage, "corrupted store file '" + _file.string() + "'");
            }
            _entries[std::move(key)] = std::move(value);
        }
        return {};
    }

    core::Result<void> FileStore::save() const {
        const std::filesystem::path temp = _file.string() + ".tmp";
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            if (!out)
                return core::fail(core::ErrorCode::storage, "cannot write '" + temp.string() + "'");

            const auto count = static_cast<std::uint64_t>(_entries.size());
            out.write(reinterpret_cast<const char*>(&count), sizeof(count));
            for (const auto& [key, value] : _entries) {
                writeSized(out, key);
                writeSized(out, value);
            }
            if (!out)
                return core::fail(core::ErrorCode::storage, "write failed on '" + temp.string() + "'");
        }

        std::error_code errorCode;
        std::filesystem::rename(temp, _file, errorCode);
        if (errorCode) {
            return core::fail(core::ErrorCode::storage, "cannot commit store file: " + errorCode.message());
        }
        return {};
    }

    core::Result<void> FileStore::put(core::BytesView key, core::BytesView value) {
        std::lock_guard lock(_mutex);
        _entries[core::Bytes(key.begin(), key.end())] = core::Bytes(value.begin(), value.end());
        return save();
    }

    core::Result<void> FileStore::erase(core::BytesView key) {
        std::lock_guard lock(_mutex);
        _entries.erase(core::Bytes(key.begin(), key.end()));
        return save();
    }

    core::Result<void> FileStore::apply(WriteBatch batch) {
        std::lock_guard lock(_mutex);
        applyLocked(batch);
        return save();
    }

}

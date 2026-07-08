#pragma once

#include <map>
#include <mutex>

#include "storage/IStorage.hpp"

namespace RSCoin::Storage {

    class MemoryStore : public IKeyValueStore {
    public:
        core::Result<std::optional<core::Bytes>> get(core::BytesView key) const override;
        core::Result<void> put(core::BytesView key, core::BytesView value) override;
        core::Result<void> erase(core::BytesView key) override;
        core::Result<void> apply(WriteBatch batch) override;
    protected:
        void applyLocked(WriteBatch& batch);

        mutable std::mutex _mutex;
        std::map<core::Bytes, core::Bytes> _entries;
    };

}

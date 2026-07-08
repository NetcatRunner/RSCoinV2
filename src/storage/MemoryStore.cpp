#include "storage/MemoryStore.hpp"

namespace RSCoin::Storage {

    namespace {
        core::Bytes toBytes(core::BytesView view) { return core::Bytes(view.begin(), view.end()); }
    }

    core::Result<std::optional<core::Bytes>> MemoryStore::get(core::BytesView key) const {
        std::lock_guard lock(_mutex);
        const auto it = _entries.find(toBytes(key));
        if (it == _entries.end())
            return std::optional<core::Bytes>{};
        return std::optional<core::Bytes>{it->second};
    }

    core::Result<void> MemoryStore::put(core::BytesView key, core::BytesView value) {
        std::lock_guard lock(_mutex);
        _entries[toBytes(key)] = toBytes(value);
        return {};
    }

    core::Result<void> MemoryStore::erase(core::BytesView key) {
        std::lock_guard lock(_mutex);
        _entries.erase(toBytes(key));
        return {};
    }

    core::Result<void> MemoryStore::apply(WriteBatch batch) {
        std::lock_guard lock(_mutex);
        applyLocked(batch);
        return {};
    }

    void MemoryStore::applyLocked(WriteBatch& batch) {
        for (auto& operation : batch.operations) {
            if (auto* putOp = std::get_if<WriteBatch::Put>(&operation))
                _entries[std::move(putOp->key)] = std::move(putOp->value);
            else if (auto* eraseOp = std::get_if<WriteBatch::Erase>(&operation))
                _entries.erase(eraseOp->key);
        }
    }

}

#include "chain/kv/Blockchain.hpp"

#include <cstdint>

#include "primitives/Codec.hpp"

namespace RSCoin::Chain {

    namespace {
        core::Bytes headKey() {
            const char raw[] = {'h', 'e', 'a', 'd'};
            return core::Bytes(reinterpret_cast<const std::byte*>(raw), reinterpret_cast<const std::byte*>(raw) + sizeof(raw));
        }

        core::Bytes hashKey(char prefix, const core::Hash256& hash) {
            core::Bytes key;
            key.reserve(2 + hash.bytes.size());
            key.push_back(static_cast<std::byte>(prefix));
            key.push_back(static_cast<std::byte>(':'));
            key.insert(key.end(), hash.bytes.begin(), hash.bytes.end());
            return key;
        }

        core::Bytes heightKey(std::uint64_t height) {
            core::Bytes key;
            key.reserve(2 + sizeof(height));
            key.push_back(static_cast<std::byte>('N'));
            key.push_back(static_cast<std::byte>(':'));
            for (unsigned shift = 0; shift < 64; shift += 8)
                key.push_back(static_cast<std::byte>((height >> shift) & 0xFF));
            return key;
        }

        core::Bytes toBytes(const core::Hash256& hash) {
            return core::Bytes(hash.bytes.begin(), hash.bytes.end());
        }

        core::Result<core::Hash256> toHash(const core::Bytes& bytes) {
            if (bytes.size() != 32)
                return core::fail(core::ErrorCode::storage, "corrupted hash entry");
            core::Hash256 hash;
            for (std::size_t i = 0; i < 32; ++i)
                hash.bytes[i] = bytes[i];
            return hash;
        }
    }

    core::Result<std::unique_ptr<Blockchain>> Blockchain::open(Storage::IKeyValueStore& store, const Crypto::IHasher& hasher, const Primitives::Block& genesis) {
        auto chain = std::unique_ptr<Blockchain>(new Blockchain(store, hasher));

        auto storedHead = store.get(headKey());
        if (!storedHead)
            return core::fail(storedHead.error(), "opening chain");

        if (!storedHead->has_value()) {
            const core::Hash256 genesisHash = hasher.hash(Primitives::encode(genesis.header));
            if (auto written = chain->writeBlock(genesis, genesisHash); !written)
                return core::fail(written.error(), "writing genesis");
            chain->_head = genesis.header;
            chain->_headHash = genesisHash;
            return chain;
        }

        auto headHash = toHash(**storedHead);
        if (!headHash)
            return core::fail(headHash.error());

        auto header = chain->headerByHash(*headHash);
        if (!header)
            return core::fail(header.error(), "loading chain head");

        chain->_head = *header;
        chain->_headHash = *headHash;
        return chain;
    }

    Primitives::BlockHeader Blockchain::head() const {
        std::lock_guard lock(_mutex);
        return _head;
    }

    core::Hash256 Blockchain::headHash() const {
        std::lock_guard lock(_mutex);
        return _headHash;
    }

    std::uint64_t Blockchain::height() const {
        std::lock_guard lock(_mutex);
        return _head.height;
    }

    core::Result<Primitives::BlockHeader> Blockchain::headerByHash(const core::Hash256& hash) const {
        auto stored = _store.get(hashKey('H', hash));
        if (!stored)
            return core::fail(stored.error());
        if (!stored->has_value())
            return core::fail(core::ErrorCode::notFound, "unknown block header");
        return Primitives::decodeHeader(**stored);
    }

    core::Result<Primitives::BlockHeader> Blockchain::headerByHeight(std::uint64_t blockHeight) const {
        auto stored = _store.get(heightKey(blockHeight));
        if (!stored)
            return core::fail(stored.error());
        if (!stored->has_value())
            return core::fail(core::ErrorCode::notFound, "no block at height " + std::to_string(blockHeight));

        auto hash = toHash(**stored);
        if (!hash)
            return core::fail(hash.error());
        return headerByHash(*hash);
    }

    core::Result<Primitives::Block> Blockchain::blockByHeight(std::uint64_t blockHeight) const {
        auto stored = _store.get(heightKey(blockHeight));
        if (!stored)
            return core::fail(stored.error());
        if (!stored->has_value())
            return core::fail(core::ErrorCode::notFound, "no block at height " + std::to_string(blockHeight));

        auto hash = toHash(**stored);
        if (!hash)
            return core::fail(hash.error());
        return blockByHash(*hash);
    }

    core::Result<Primitives::Block> Blockchain::blockByHash(const core::Hash256& hash) const {
        auto stored = _store.get(hashKey('B', hash));
        if (!stored)
            return core::fail(stored.error());
        if (!stored->has_value())
            return core::fail(core::ErrorCode::notFound, "unknown block");
        return Primitives::decodeBlock(**stored);
    }

    core::Result<void> Blockchain::appendBlock(const Primitives::Block& block) {
        std::lock_guard lock(_mutex);

        if (block.header.parentHash != _headHash)
            return core::fail(core::ErrorCode::validation, "block does not extend the current head");
        if (block.header.height != _head.height + 1)
            return core::fail(core::ErrorCode::validation, "unexpected block height");

        const core::Hash256 hash = _hasher.hash(Primitives::encode(block.header));
        if (auto written = writeBlock(block, hash); !written)
            return core::fail(written.error(), "appending block");

        _head = block.header;
        _headHash = hash;
        return {};
    }

    core::Result<void> Blockchain::writeBlock(const Primitives::Block& block, const core::Hash256& hash) {
        Storage::WriteBatch batch;
        batch.put(hashKey('H', hash), Primitives::encode(block.header));
        batch.put(hashKey('B', hash), Primitives::encode(block));
        batch.put(heightKey(block.header.height), toBytes(hash));
        batch.put(headKey(), toBytes(hash));
        return _store.apply(std::move(batch));
    }

}

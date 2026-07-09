#pragma once

#include <memory>
#include <mutex>

#include "chain/IBlockchain.hpp"
#include "crypto/ICrypto.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Chain {

    class Blockchain : public IBlockchain {
    public:
        static core::Result<std::unique_ptr<Blockchain>> open(Storage::IKeyValueStore& store, const Crypto::IHasher& hasher, const Primitives::Block& genesis);

        Primitives::BlockHeader head() const override;
        core::Result<Primitives::BlockHeader> headerByHash(const core::Hash256& hash) const override;
        core::Result<Primitives::BlockHeader> headerByHeight(std::uint64_t height) const override;

        core::Result<void> appendBlock(const Primitives::Block& block) override;
        core::Result<Primitives::Block> blockByHash(const core::Hash256& hash) const override;
        core::Result<Primitives::Block> blockByHeight(std::uint64_t height) const override;

        core::Hash256 headHash() const override;
        std::uint64_t height() const override;

    private:
        Blockchain(Storage::IKeyValueStore& store, const Crypto::IHasher& hasher): _store(store), _hasher(hasher) {}

        core::Result<void> writeBlock(const Primitives::Block& block, const core::Hash256& hash);

        Storage::IKeyValueStore& _store;
        const Crypto::IHasher& _hasher;

        mutable std::mutex _mutex;
        Primitives::BlockHeader _head;
        core::Hash256 _headHash;
    };

}

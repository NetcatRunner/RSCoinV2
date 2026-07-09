#pragma once

#include <vector>

#include "core/Result.hpp"
#include "core/Types.hpp"
#include "storage/IStorage.hpp"

namespace RSCoin::Wallet {

    class Keystore {
    public:
        Keystore(Storage::IKeyValueStore& store) : _store(store) {}

        core::Result<void> save(const core::Address& address, const core::KeyPair& keys);
        core::Result<core::KeyPair> load(const core::Address& address) const;
        core::Result<std::vector<core::Address>> list() const;

    private:
        Storage::IKeyValueStore& _store;
    };

}

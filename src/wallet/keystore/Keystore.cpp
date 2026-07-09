#include "wallet/keystore/Keystore.hpp"

#include <string>

#include "core/Hex.hpp"

namespace RSCoin::Wallet {

    namespace {
        constexpr std::string_view kIndexKey = "accounts";
        constexpr char kSeparator = ':';

        core::Bytes toBytes(std::string_view text) {
            return {reinterpret_cast<const std::byte*>(text.data()),
                    reinterpret_cast<const std::byte*>(text.data()) + text.size()};
        }

        std::string toText(const core::Bytes& bytes) {
            return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
        }

        core::Bytes accountKey(const core::Address& address) {
            return toBytes("K:" + core::toHex(address));
        }
    }

    core::Result<void> Keystore::save(const core::Address& address, const core::KeyPair& keys) {
        const std::string record = core::toHex(core::BytesView{keys.privateKey.data}) + kSeparator + core::toHex(core::BytesView{keys.publicKey.data});
        if (auto put = _store.put(accountKey(address), toBytes(record)); !put)
            return core::fail(put.error(), "saving key");

        auto known = list();
        if (!known)
            return core::fail(known.error());
        std::string index;
        for (const auto& entry : *known)
            index += core::toHex(entry) + "\n";
        index += core::toHex(address) + "\n";
        return _store.put(toBytes(kIndexKey), toBytes(index));
    }

    core::Result<core::KeyPair> Keystore::load(const core::Address& address) const {
        const auto record = _store.get(accountKey(address));
        if (!record)
            return core::fail(record.error());
        if (!*record)
            return core::fail(core::ErrorCode::notFound, "no key for address " + core::toHex(address));

        const std::string text = toText(**record);
        const auto separator = text.find(kSeparator);
        if (separator == std::string::npos)
            return core::fail(core::ErrorCode::storage, "corrupted keystore record");

        auto privateKey = core::fromHex(text.substr(0, separator));
        auto publicKey = core::fromHex(text.substr(separator + 1));
        if (!privateKey || !publicKey)
            return core::fail(core::ErrorCode::storage, "corrupted keystore record");
        return core::KeyPair{core::PrivateKey{std::move(*privateKey)}, core::PublicKey{std::move(*publicKey)}};
    }

    core::Result<std::vector<core::Address>> Keystore::list() const {
        const auto index = _store.get(toBytes(kIndexKey));
        if (!index)
            return core::fail(index.error());

        std::vector<core::Address> addresses;
        if (!*index)
            return addresses;

        const std::string text = toText(**index);
        std::size_t start = 0;
        while (start < text.size()) {
            const auto end = text.find('\n', start);
            if (end == std::string::npos)
                break;
            auto address = core::fixedFromHex<20>(text.substr(start, end - start));
            if (!address)
                return core::fail(address.error(), "corrupted keystore index");
            addresses.push_back(*address);
            start = end + 1;
        }
        return addresses;
    }

}

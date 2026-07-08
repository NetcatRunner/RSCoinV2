#include "crypto/Suite.hpp"

#include <random>

#include "crypto/Sha256.hpp"

namespace RSCoin::Crypto {

    namespace {
        constexpr std::size_t kStubKeySize = 32;

        core::Hash256 toHash(const Sha256::Digest& digest) {
            core::Hash256 out;
            for (std::size_t i = 0; i < digest.size(); ++i)
                out.bytes[i] = static_cast<std::byte>(digest[i]);
            return out;
        }
    }

    core::Hash256 Sha256dHasher::hash(core::BytesView data) const {
        const auto first = Sha256::hashRaw(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        return toHash(Sha256::hashRaw(first.data(), first.size()));
    }

    core::Result<core::KeyPair> InsecureStubScheme::generateKeyPair() const {
        std::random_device entropy;

        core::PrivateKey privateKey;
        privateKey.data.reserve(kStubKeySize);
        for (std::size_t i = 0; i < kStubKeySize; i += 4) {
            const auto word = entropy();
            for (unsigned shift = 0; shift < 32; shift += 8) {
                privateKey.data.push_back(static_cast<std::byte>((word >> shift) & 0xFF));
            }
        }

        core::PublicKey publicKey{privateKey.data};
        return core::KeyPair{std::move(privateKey), std::move(publicKey)};
    }

    core::Result<core::Signature> InsecureStubScheme::sign(const core::Hash256& digest, const core::PrivateKey& key) const {
        if (key.data.size() != kStubKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid private key size");

        core::Bytes material(digest.bytes.begin(), digest.bytes.end());
        material.insert(material.end(), key.data.begin(), key.data.end());

        const core::Hash256 mac = _hasher.hash(material);
        return core::Signature{core::Bytes(mac.bytes.begin(), mac.bytes.end())};
    }

    bool InsecureStubScheme::verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const {
        if (key.data.size() != kStubKeySize)
            return false;

        core::Bytes material(digest.bytes.begin(), digest.bytes.end());
        material.insert(material.end(), key.data.begin(), key.data.end());

        const core::Hash256 expected = _hasher.hash(material);
        return signature.data == core::Bytes(expected.bytes.begin(), expected.bytes.end());
    }

    core::Result<core::Address> InsecureStubScheme::deriveAddress(const core::PublicKey& key) const {
        if (key.data.size() != kStubKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid public key size");

        const core::Hash256 digest = _hasher.hash(key.data);
        core::Address address;
        for (std::size_t i = 0; i < address.bytes.size(); ++i)
            address.bytes[i] = digest.bytes[digest.bytes.size() - address.bytes.size() + i];
        return address;
    }

}

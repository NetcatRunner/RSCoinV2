#include "crypto/stub/InsecureStubScheme.hpp"

#include <algorithm>
#include <random>

namespace RSCoin::Crypto {

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

    core::Hash256 InsecureStubScheme::computeMac(const core::Hash256& digest, core::BytesView key) const {
        core::Bytes material(digest.bytes.begin(), digest.bytes.end());
        material.insert(material.end(), key.begin(), key.end());
        return _hasher.hash(material);
    }

    core::Result<core::Signature> InsecureStubScheme::sign(const core::Hash256& digest, const core::PrivateKey& key) const {
        if (key.data.size() != kStubKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid private key size");

        const core::Hash256 mac = computeMac(digest, key.data);

        core::Bytes blob(key.data);
        blob.insert(blob.end(), mac.bytes.begin(), mac.bytes.end());
        return core::Signature{std::move(blob)};
    }

    bool InsecureStubScheme::verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const {
        if (signature.data.size() != kStubKeySize + kMacSize || key.data.size() != kStubKeySize)
            return false;
        if (!std::equal(key.data.begin(), key.data.end(), signature.data.begin()))
            return false;

        const core::Hash256 expected = computeMac(digest, key.data);
        return std::equal(expected.bytes.begin(), expected.bytes.end(), signature.data.begin() + kStubKeySize);
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

    core::Result<core::Address> InsecureStubScheme::authenticate(const core::Hash256& digest, const core::Signature& signature) const {
        if (signature.data.size() != kStubKeySize + kMacSize)
            return core::fail(core::ErrorCode::crypto, "malformed signature blob");

        core::PublicKey publicKey{core::Bytes(signature.data.begin(), signature.data.begin() + kStubKeySize)};
        if (!verify(digest, signature, publicKey))
            return core::fail(core::ErrorCode::crypto, "signature verification failed");
        return deriveAddress(publicKey);
    }

}

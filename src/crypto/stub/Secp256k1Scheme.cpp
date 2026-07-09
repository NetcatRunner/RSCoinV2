#include "crypto/stub/Secp256k1Scheme.hpp"

#include <random>

#include <secp256k1.h>

namespace RSCoin::Crypto {

    namespace {
        void fillRandom(core::Bytes& out, std::size_t size) {
            std::random_device entropy;
            out.resize(size);
            for (std::size_t i = 0; i < size; i += 4) {
                const auto word = entropy();
                for (unsigned shift = 0; shift < 32 && i + shift / 8 < size; shift += 8)
                    out[i + shift / 8] = static_cast<std::byte>((word >> shift) & 0xFF);
            }
        }
    }

    core::Result<core::KeyPair> Secp256k1Scheme::generateKeyPair() const {
        secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

        core::PrivateKey privateKey;
        do {
            fillRandom(privateKey.data, kStubKeySize);
        } while (!secp256k1_ec_seckey_verify(ctx, reinterpret_cast<const unsigned char*>(privateKey.data.data())));

        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(ctx, &pubkey, reinterpret_cast<const unsigned char*>(privateKey.data.data()))) {
            secp256k1_context_destroy(ctx);
            return core::fail(core::ErrorCode::crypto, "failed to generate public key");
        }

        core::PublicKey publicKey;
        publicKey.data.resize(33);
        std::size_t length = publicKey.data.size();
        secp256k1_ec_pubkey_serialize(ctx, reinterpret_cast<unsigned char*>(publicKey.data.data()), &length, &pubkey, SECP256K1_EC_COMPRESSED);

        secp256k1_context_destroy(ctx);
        return core::KeyPair{std::move(privateKey), std::move(publicKey)};
    }

    core::Result<core::Signature> Secp256k1Scheme::sign(const core::Hash256& digest, const core::PrivateKey& key) const {
        if (key.data.size() != kStubKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid private key size");

        core::Bytes material(digest.bytes.begin(), digest.bytes.end());
        material.insert(material.end(), key.data.begin(), key.data.end());

        const core::Hash256 mac = _hasher.hash(material);
        return core::Signature{core::Bytes(mac.bytes.begin(), mac.bytes.end())};
    }

    bool Secp256k1Scheme::verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const {
        if (key.data.size() != kStubKeySize)
            return false;

        core::Bytes material(digest.bytes.begin(), digest.bytes.end());
        material.insert(material.end(), key.data.begin(), key.data.end());

        const core::Hash256 expected = _hasher.hash(material);
        return signature.data == core::Bytes(expected.bytes.begin(), expected.bytes.end());
    }

    core::Result<core::Address> Secp256k1Scheme::deriveAddress(const core::PublicKey& key) const {
        if (key.data.size() != kStubKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid public key size");

        const core::Hash256 digest = _hasher.hash(key.data);
        core::Address address;
        for (std::size_t i = 0; i < address.bytes.size(); ++i)
            address.bytes[i] = digest.bytes[digest.bytes.size() - address.bytes.size() + i];
        return address;
    }

}

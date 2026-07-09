#include "crypto/secp256k1/Secp256k1Scheme.hpp"

#include <algorithm>
#include <random>

#include <secp256k1.h>

namespace RSCoin::Crypto {

    namespace {
        const secp256k1_context* context() {
            static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
            return ctx;
        }

        const unsigned char* raw(core::BytesView data) {
            return reinterpret_cast<const unsigned char*>(data.data());
        }

        unsigned char* raw(core::Bytes& data) {
            return reinterpret_cast<unsigned char*>(data.data());
        }

        const unsigned char* raw(const core::Hash256& digest) {
            return reinterpret_cast<const unsigned char*>(digest.bytes.data());
        }

        void fillRandom(core::Bytes& out, std::size_t size) {
            std::random_device entropy;
            out.resize(size);
            for (std::size_t i = 0; i < size; i += 4) {
                const auto word = entropy();
                for (unsigned shift = 0; shift < 32 && i + shift / 8 < size; shift += 8) {
                    out[i + shift / 8] = static_cast<std::byte>((word >> shift) & 0xFF);
                }
            }
        }

        core::Result<core::PublicKey> compressedFrom(const secp256k1_pubkey& pubkey) {
            core::PublicKey publicKey;
            publicKey.data.resize(33);
            std::size_t length = publicKey.data.size();
            if (!secp256k1_ec_pubkey_serialize(context(), raw(publicKey.data), &length, &pubkey, SECP256K1_EC_COMPRESSED))
                return core::fail(core::ErrorCode::crypto, "failed to serialize public key");
            return publicKey;
        }
    }

    core::Result<core::KeyPair> Secp256k1Scheme::generateKeyPair() const {
        core::PrivateKey privateKey;
        do {
            fillRandom(privateKey.data, kPrivateKeySize);
        } while (!secp256k1_ec_seckey_verify(context(), raw(privateKey.data)));

        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(context(), &pubkey, raw(privateKey.data)))
            return core::fail(core::ErrorCode::crypto, "failed to generate public key");

        auto publicKey = compressedFrom(pubkey);
        if (!publicKey)
            return core::fail(publicKey.error());
        return core::KeyPair{std::move(privateKey), std::move(*publicKey)};
    }

    core::Result<core::Signature> Secp256k1Scheme::sign(const core::Hash256& digest, const core::PrivateKey& key) const {
        if (key.data.size() != kPrivateKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid private key size");

        secp256k1_ecdsa_signature ecdsaSig;
        if (!secp256k1_ecdsa_sign(context(), &ecdsaSig, raw(digest), raw(key.data), nullptr, nullptr))
            return core::fail(core::ErrorCode::crypto, "signing failed");

        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(context(), &pubkey, raw(key.data)))
            return core::fail(core::ErrorCode::crypto, "failed to derive public key");
        auto publicKey = compressedFrom(pubkey);
        if (!publicKey)
            return core::fail(publicKey.error());

        core::Bytes blob(std::move(publicKey->data));
        blob.resize(kPublicKeySize + kCompactSigSize);
        if (!secp256k1_ecdsa_signature_serialize_compact(context(), raw(blob) + kPublicKeySize, &ecdsaSig))
            return core::fail(core::ErrorCode::crypto, "failed to serialize signature");
        return core::Signature{std::move(blob)};
    }

    bool Secp256k1Scheme::verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const {
        if (signature.data.size() != kPublicKeySize + kCompactSigSize || key.data.size() != kPublicKeySize)
            return false;
        if (!std::equal(key.data.begin(), key.data.end(), signature.data.begin()))
            return false;

        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_parse(context(), &pubkey, raw(key.data), key.data.size()))
            return false;

        secp256k1_ecdsa_signature ecdsaSig;
        if (!secp256k1_ecdsa_signature_parse_compact(context(), &ecdsaSig, raw(signature.data) + kPublicKeySize))
            return false;

        return secp256k1_ecdsa_verify(context(), &ecdsaSig, raw(digest), &pubkey) == 1;
    }

    core::Result<core::Address> Secp256k1Scheme::deriveAddress(const core::PublicKey& key) const {
        if (key.data.size() != kPublicKeySize)
            return core::fail(core::ErrorCode::crypto, "invalid public key size");

        const core::Hash256 digest = _hasher.hash(key.data);
        core::Address address;
        for (std::size_t i = 0; i < address.bytes.size(); ++i)
            address.bytes[i] = digest.bytes[digest.bytes.size() - address.bytes.size() + i];
        return address;
    }

    core::Result<core::Address> Secp256k1Scheme::authenticate(const core::Hash256& digest, const core::Signature& signature) const {
        if (signature.data.size() != kPublicKeySize + kCompactSigSize)
            return core::fail(core::ErrorCode::crypto, "malformed signature blob");

        core::PublicKey publicKey{core::Bytes(signature.data.begin(), signature.data.begin() + kPublicKeySize)};
        if (!verify(digest, signature, publicKey))
            return core::fail(core::ErrorCode::crypto, "signature verification failed");
        return deriveAddress(publicKey);
    }

}

#pragma once

#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {

    class Secp256k1Scheme : public ISignatureScheme {
    public:
        Secp256k1Scheme(const IHasher& hasher) : _hasher(hasher) {}

        core::Result<core::KeyPair> generateKeyPair() const override;
        core::Result<core::Signature> sign(const core::Hash256& digest, const core::PrivateKey& key) const override;
        bool verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const override;
        core::Result<core::Address> deriveAddress(const core::PublicKey& key) const override;
        core::Result<core::Address> authenticate(const core::Hash256& digest, const core::Signature& signature) const override;

    private:
        static constexpr std::size_t kPrivateKeySize = 32;
        static constexpr std::size_t kPublicKeySize = 33;
        static constexpr std::size_t kCompactSigSize = 64;

        const IHasher& _hasher;
    };

}

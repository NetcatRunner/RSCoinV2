#pragma once

#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {

    class Secp256k1Scheme : public ISignatureScheme {
    public:
        Secp256k1Scheme(const IHasher& hasher) : _hasher(hasher) {}
        Secp256k1Scheme();

        core::Result<core::KeyPair> generateKeyPair() const override;
        core::Result<core::Signature> sign(const core::Hash256& digest, const core::PrivateKey& key) const override;
        bool verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const override;
        core::Result<core::Address> deriveAddress(const core::PublicKey& key) const override;

    private:
        static constexpr std::size_t kStubKeySize = 32;
        const IHasher& _hasher;
    };

}

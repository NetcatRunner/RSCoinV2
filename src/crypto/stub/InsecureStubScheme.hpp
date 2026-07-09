#pragma once

#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {

    class InsecureStubScheme : public ISignatureScheme {
    public:
        InsecureStubScheme(const IHasher& hasher) : _hasher(hasher) {}

        core::Result<core::KeyPair> generateKeyPair() const override;
        core::Result<core::Signature> sign(const core::Hash256& digest, const core::PrivateKey& key) const override;
        bool verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const override;
        core::Result<core::Address> deriveAddress(const core::PublicKey& key) const override;

    private:
        const IHasher& _hasher;
    };

}

#pragma once

#include <memory>

#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {

    class Sha256dHasher : public IHasher {
    public:
        core::Hash256 hash(core::BytesView data) const override;
    };

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

    class CryptoSuite : public ICryptoProvider {
    public:
        CryptoSuite(std::unique_ptr<IHasher> hasher, std::unique_ptr<ISignatureScheme> signatures)
            : _hasher(std::move(hasher)), _signatures(std::move(signatures)) {}

        const IHasher& hasher() const noexcept override { return *_hasher; }
        const ISignatureScheme& signatures() const noexcept override { return *_signatures; }

    private:
        std::unique_ptr<IHasher> _hasher;
        std::unique_ptr<ISignatureScheme> _signatures;
    };

}

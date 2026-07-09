#pragma once

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Crypto {

    class IHasher {
    public:
        virtual ~IHasher() = default;

        virtual core::Hash256 hash(core::BytesView data) const = 0;
    };

    class ISignatureScheme {
    public:
        virtual ~ISignatureScheme() = default;

        virtual core::Result<core::KeyPair> generateKeyPair() const = 0;
        virtual core::Result<core::Signature> sign(const core::Hash256& digest, const core::PrivateKey& key) const = 0;
        virtual bool verify(const core::Hash256& digest, const core::Signature& signature, const core::PublicKey& key) const = 0;
        virtual core::Result<core::Address> deriveAddress(const core::PublicKey& key) const = 0;
        virtual core::Result<core::Address> authenticate(const core::Hash256& digest, const core::Signature& signature) const = 0;
    };

    class ICryptoProvider {
    public:
        virtual ~ICryptoProvider() = default;

        virtual const IHasher& hasher() const noexcept = 0;
        virtual const ISignatureScheme& signatures() const noexcept = 0;
    };

}

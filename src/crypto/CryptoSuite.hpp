#pragma once

#include <memory>

#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {
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

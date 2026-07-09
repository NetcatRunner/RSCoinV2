#include "crypto/Factory.hpp"

#include "crypto/CryptoSuite.hpp"
#include "crypto/sha256/Sha256dHasher.hpp"

#include "crypto/stub/Secp256k1Scheme.hpp"
#include "crypto/stub/InsecureStubScheme.hpp"

namespace RSCoin::Crypto {

    core::Result<std::unique_ptr<ICryptoProvider>> makeProvider(const CryptoConfig& config) {
        std::unique_ptr<IHasher> hasher;
        if (config.hasher == "sha256d")
            hasher = std::make_unique<Sha256dHasher>();
        else
            return core::fail(core::ErrorCode::crypto, "unknown hasher: '" + config.hasher + "'");

        std::unique_ptr<ISignatureScheme> signatures;
        if (config.signatureScheme == "insecure-stub")
            signatures = std::make_unique<InsecureStubScheme>(*hasher);
        else if (config.signatureScheme == "secp256k1")
            signatures = std::make_unique<Secp256k1Scheme>(*hasher);
        else
            return core::fail(core::ErrorCode::crypto, "unknown signature scheme: '" + config.signatureScheme + "' (secp256k1 not implemented yet)");

        return std::make_unique<CryptoSuite>(std::move(hasher), std::move(signatures));
    }

}

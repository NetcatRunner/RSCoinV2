#pragma once

#include "crypto/ICrypto.hpp"

namespace RSCoin::Crypto {

    class Sha256dHasher : public IHasher {
    public:
        core::Hash256 hash(core::BytesView data) const override;
    };

}

#include "crypto/sha256/Sha256dHasher.hpp"

#include "crypto/sha256/Sha256.hpp"

namespace RSCoin::Crypto {

    namespace {
        core::Hash256 toHash(const Sha256::Digest& digest) {
            core::Hash256 out;
            for (std::size_t i = 0; i < digest.size(); ++i) {
                out.bytes[i] = static_cast<std::byte>(digest[i]);
            }
            return out;
        }
    }

    core::Hash256 Sha256dHasher::hash(core::BytesView data) const {
        const auto first = Sha256::hashRaw(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        return toHash(Sha256::hashRaw(first.data(), first.size()));
    }

}

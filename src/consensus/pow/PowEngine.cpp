#include "consensus/pow/PowEngine.hpp"

#include <charconv>
#include <limits>

#include "primitives/Codec.hpp"

namespace RSCoin::Consensus {

    namespace {
        constexpr std::size_t kSealSize = 8;

        void writeNonce(core::Bytes& seal, std::uint64_t nonce) {
            seal.resize(kSealSize);
            for (unsigned shift = 0; shift < 64; shift += 8)
                seal[shift / 8] = static_cast<std::byte>((nonce >> shift) & 0xFF);
        }

        unsigned leadingZeroBits(const core::Hash256& hash) {
            unsigned bits = 0;
            for (const std::byte b : hash.bytes) {
                const auto value = std::to_integer<unsigned>(b);
                if (value == 0) {
                    bits += 8;
                    continue;
                }
                for (unsigned mask = 0x80; mask != 0 && (value & mask) == 0; mask >>= 1)
                    ++bits;
                break;
            }
            return bits;
        }
    }

    core::Result<std::unique_ptr<PowEngine>> PowEngine::create(const std::map<std::string, std::string>& parameters, const Crypto::IHasher& hasher) {
        const auto it = parameters.find("difficultyBits");
        if (it == parameters.end())
            return core::fail(core::ErrorCode::consensus, "pow engine requires parameter 'difficultyBits'");

        unsigned difficultyBits{};
        const auto [ptr, ec] = std::from_chars(it->second.data(), it->second.data() + it->second.size(), difficultyBits);
        if (ec != std::errc{} || difficultyBits == 0 || difficultyBits > 255)
            return core::fail(core::ErrorCode::consensus, "invalid 'difficultyBits' value: '" + it->second + "'");

        return std::unique_ptr<PowEngine>(new PowEngine(hasher, difficultyBits));
    }

    bool PowEngine::meetsDifficulty(const core::Hash256& hash) const {
        return leadingZeroBits(hash) >= _difficultyBits;
    }

    core::Result<void> PowEngine::verify(const Primitives::BlockHeader& header, const Chain::IChainView& chain) const {
        auto parent = chain.headerByHash(header.parentHash);
        if (!parent)
            return core::fail(core::ErrorCode::consensus, "unknown parent block");

        if (header.height != parent->height + 1)
            return core::fail(core::ErrorCode::consensus, "height does not follow parent");
        if (header.timestamp <= parent->timestamp)
            return core::fail(core::ErrorCode::consensus, "timestamp not after parent");
        if (header.consensusSeal.size() != kSealSize)
            return core::fail(core::ErrorCode::consensus, "malformed pow seal");

        if (!meetsDifficulty(_hasher.hash(Primitives::encode(header))))
            return core::fail(core::ErrorCode::consensus, "insufficient proof of work");
        return {};
    }

    core::Result<void> PowEngine::prepare(Primitives::BlockHeader& draft, const Chain::IChainView& chain) const {
        const Primitives::BlockHeader parent = chain.head();
        if (draft.timestamp <= parent.timestamp)
            draft.timestamp = parent.timestamp + 1;
        writeNonce(draft.consensusSeal, 0);
        return {};
    }

    core::Result<Primitives::Block> PowEngine::seal(Primitives::Block draft, const Chain::IChainView& /*chain*/, std::stop_token cancel) const {
        for (std::uint64_t nonce = 0;; ++nonce) {
            if (cancel.stop_requested())
                return core::fail(core::ErrorCode::cancelled, "sealing cancelled");

            writeNonce(draft.header.consensusSeal, nonce);
            if (meetsDifficulty(_hasher.hash(Primitives::encode(draft.header))))
                return draft;

            if (nonce == std::numeric_limits<std::uint64_t>::max())
                return core::fail(core::ErrorCode::consensus, "nonce space exhausted");
        }
    }

    core::Result<std::strong_ordering> PowEngine::compare(const Primitives::BlockHeader& lhs, const Primitives::BlockHeader& rhs, const Chain::IChainView& /*chain*/) const {
        if (lhs.height != rhs.height)
            return lhs.height <=> rhs.height;

        const core::Hash256 lhsHash = _hasher.hash(Primitives::encode(lhs));
        const core::Hash256 rhsHash = _hasher.hash(Primitives::encode(rhs));
        return rhsHash.bytes <=> lhsHash.bytes;
    }

}

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "consensus/IConsensus.hpp"
#include "crypto/ICrypto.hpp"

namespace RSCoin::Consensus {

    class PowEngine final : public IConsensus {
    public:
        static core::Result<std::unique_ptr<PowEngine>>
        create(const std::map<std::string, std::string>& parameters, const Crypto::IHasher& hasher);

        std::string_view name() const noexcept override { return "pow"; }

        core::Result<void> verify(const Primitives::BlockHeader& header, const Chain::IChainView& chain) const override;
        core::Result<void> prepare(Primitives::BlockHeader& draft, const Chain::IChainView& chain) const override;
        core::Result<Primitives::Block> seal(Primitives::Block draft, const Chain::IChainView& chain, std::stop_token cancel) const override;
        core::Result<std::strong_ordering> compare(const Primitives::BlockHeader& lhs, const Primitives::BlockHeader& rhs, const Chain::IChainView& chain) const override;

    private:
        PowEngine(const Crypto::IHasher& hasher, unsigned difficultyBits)
            : _hasher(hasher), _difficultyBits(difficultyBits) {}

        bool meetsDifficulty(const core::Hash256& hash) const;

        const Crypto::IHasher& _hasher;
        unsigned _difficultyBits;
    };

}

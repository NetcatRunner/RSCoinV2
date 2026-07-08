#pragma once

#include <functional>
#include <memory>

#include "core/Result.hpp"
#include "primitives/Block.hpp"
#include "state/IStateMachine.hpp"

namespace RSCoin::Chain {

    enum class ImportOrigin { local, remote };

    enum class ImportOutcome {
        imported,
        alreadyKnown,
        orphaned,
        forked,
        invalid,
    };

    class IChainManager {
    public:
        using BlockSubscriber = std::function<void(const Primitives::Block&, ImportOrigin)>;

        virtual ~IChainManager() = default;

        virtual core::Result<ImportOutcome> importBlock(const Primitives::Block& block, ImportOrigin origin) = 0;

        virtual std::shared_ptr<const State::IStateMachine> ledger() const = 0;

        virtual void subscribe(BlockSubscriber subscriber) = 0;
    };

}

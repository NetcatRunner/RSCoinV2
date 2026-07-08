#pragma once

#include <memory>

#include "core/Result.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::State {

    class IStateMachine {
    public:
        virtual ~IStateMachine() = default;

        virtual core::Result<void> validateTransaction(const Primitives::Transaction& transaction) const = 0;

        virtual core::Result<std::unique_ptr<IStateMachine>> apply(const Primitives::Block& block) const = 0;

        virtual core::Hash256 stateRoot() const = 0;
    };

}

#pragma once

#include <memory>
#include <optional>

#include "core/Result.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::State {

    struct AccountView {
        core::Amount balance;
        std::uint64_t nonce{};
    };

    class IStateMachine {
    public:
        virtual ~IStateMachine() = default;

        virtual core::Result<void> validateTransaction(const Primitives::Transaction& transaction) const = 0;

        virtual core::Result<std::unique_ptr<IStateMachine>> apply(const Primitives::Block& block) const = 0;

        virtual std::optional<AccountView> account(const core::Address& address) const = 0;

        virtual core::Hash256 stateRoot() const = 0;
    };

}

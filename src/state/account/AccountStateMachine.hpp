#pragma once

#include <map>

#include "chain/GenesisConfig.hpp"
#include "state/StateConfig.hpp"
#include "crypto/ICrypto.hpp"
#include "state/IStateMachine.hpp"

namespace RSCoin::State {

    class AccountStateMachine : public IStateMachine {
    public:
        AccountStateMachine(const Crypto::IHasher& hasher, const StateConfig& rules, const Chain::GenesisConfig& genesis);

        core::Result<void> validateTransaction(const Primitives::Transaction& transaction) const override;
        core::Result<std::unique_ptr<IStateMachine>> apply(const Primitives::Block& block) const override;
        core::Hash256 stateRoot() const override;

    private:
        struct Account {
            core::Amount balance;
            std::uint64_t nonce{};
        };

        using Accounts = std::map<core::Address, Account>;

        AccountStateMachine(const Crypto::IHasher& hasher, core::Amount blockReward, Accounts accounts)
            : _hasher(hasher), _blockReward(blockReward), _accounts(std::move(accounts)) {}

        static core::Result<void> validateAgainst(const Accounts& accounts, const Primitives::Transaction& transaction);

        const Crypto::IHasher& _hasher;
        core::Amount _blockReward;
        Accounts _accounts;
    };

}

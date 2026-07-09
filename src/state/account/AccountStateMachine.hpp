#pragma once

#include <map>

#include "chain/GenesisConfig.hpp"
#include "crypto/ICrypto.hpp"
#include "state/IStateMachine.hpp"
#include "state/StateConfig.hpp"

namespace RSCoin::State {

    class AccountStateMachine : public IStateMachine {
    public:
        struct Context {
            const Crypto::IHasher& hasher;
            const Crypto::ISignatureScheme& signatures;
            std::uint64_t chainId{};
        };

        AccountStateMachine(Context context, const StateConfig& rules, const Chain::GenesisConfig& genesis);

        core::Result<void> validateTransaction(const Primitives::Transaction& transaction) const override;
        core::Result<std::unique_ptr<IStateMachine>> apply(const Primitives::Block& block) const override;
        std::optional<AccountView> account(const core::Address& address) const override;
        core::Hash256 stateRoot() const override;

    private:
        struct Account {
            core::Amount balance;
            std::uint64_t nonce{};
        };

        using Accounts = std::map<core::Address, Account>;

        AccountStateMachine(Context context, core::Amount blockReward, Accounts accounts)
            : _context(context), _blockReward(blockReward), _accounts(std::move(accounts)) {}

        core::Result<void> validateAgainst(const Accounts& accounts, const Primitives::Transaction& transaction) const;

        Context _context;
        core::Amount _blockReward;
        Accounts _accounts;
    };

}

#include "state/Factory.hpp"

#include "state/account/AccountStateMachine.hpp"

namespace RSCoin::State {

    core::Result<std::unique_ptr<IStateMachine>> makeStateMachine(const Crypto::ICryptoProvider& crypto, std::uint64_t chainId, const StateConfig& rules, const Chain::GenesisConfig& genesis) {
        const AccountStateMachine::Context context{crypto.hasher(), crypto.signatures(), chainId};
        return std::make_unique<AccountStateMachine>(context, rules, genesis);
    }

}

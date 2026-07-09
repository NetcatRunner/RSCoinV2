#include "state/Factory.hpp"

#include "state/account/AccountStateMachine.hpp"

namespace RSCoin::State {

    core::Result<std::unique_ptr<IStateMachine>> makeStateMachine(const Crypto::ICryptoProvider& crypto, const StateConfig& rules, const Chain::GenesisConfig& genesis) {
        return std::make_unique<AccountStateMachine>(crypto.hasher(), rules, genesis);
    }

}

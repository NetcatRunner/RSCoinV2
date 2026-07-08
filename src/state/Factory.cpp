#include "state/Factory.hpp"

#include "state/AccountStateMachine.hpp"

namespace RSCoin::State {

    core::Result<std::unique_ptr<IStateMachine>> makeStateMachine(const Crypto::ICryptoProvider& crypto, const Config::NodeConfig& config) {
        return std::make_unique<AccountStateMachine>(crypto.hasher(), config.state, config.genesis);
    }

}

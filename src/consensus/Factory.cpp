#include "consensus/EngineFactory.hpp"

#include "consensus/pow/PowEngine.hpp"

namespace RSCoin::Consensus {

    core::Result<std::unique_ptr<IConsensus>> makeEngine(const Config::ConsensusConfig& config, const Crypto::ICryptoProvider& crypto) {
        if (config.engine == "pow") {
            auto engine = PowEngine::create(config.parameters, crypto.hasher());
            if (!engine)
                return core::fail(engine.error());
            return std::unique_ptr<IConsensus>(std::move(*engine));
        }
        return core::fail(core::ErrorCode::consensus, "unknown consensus engine: '" + config.engine + "'");
    }

}

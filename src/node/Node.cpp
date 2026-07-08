#include "Node.hpp"

#include <chrono>
#include <utility>

#include <RST/time/FrameTimer.hpp>

#include "core/Hex.hpp"
#include "log/Logger.hpp"

namespace RSCoin::Node {

    namespace {
        std::uint64_t nowSeconds() {
            return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()) .count());
        }
    }

    Node::Node(Modules modules, Config::NodeConfig config): _modules(std::move(modules)), _config(std::move(config)) {
        _modules.manager->subscribe([this](const Primitives::Block& block, Chain::ImportOrigin) {
            _modules.mempool->removeIncluded(block);
        });
    }

    core::Result<void> Node::run(std::stop_token stop) {
        RSCoin_INFO("chain head: height {} ({})", _modules.chain->height(), core::toHex(_modules.chain->headHash()));

        if (auto started = _modules.network->start(*_modules.protocol); !started)
            return started;

        auto frameTimer = RST::Time::FrameTimer(10);
        while (!stop.stop_requested()) {
            _modules.protocol->tick();

            if (_config.mining.enabled) {
                if (auto mined = mineOne(stop); !mined && mined.error().code != core::ErrorCode::cancelled) {
                    RSCoin_WARN("mining failed: {}", mined.error().describe());
                }
            }

            frameTimer.tick();
        }

        _modules.network->stop();
        return {};
    }

    core::Result<void> Node::mineOne(std::stop_token stop) {
        Primitives::Block draft;
        draft.header.parentHash = _modules.chain->headHash();
        draft.header.height = _modules.chain->height() + 1;
        draft.header.timestamp = nowSeconds();
        draft.header.beneficiary = _config.mining.beneficiary;
        draft.transactions = _modules.mempool->take(_config.block.maxTransactions);

        if (auto prepared = _modules.consensus->prepare(draft.header, *_modules.chain); !prepared)
            return core::fail(prepared.error(), "preparing block");

        auto preview = _modules.manager->ledger()->apply(draft);
        if (!preview)
            return core::fail(preview.error(), "executing draft");
        draft.header.stateRoot = (*preview)->stateRoot();

        auto sealed = _modules.consensus->seal(std::move(draft), *_modules.chain, stop);
        if (!sealed)
            return core::fail(sealed.error());

        auto outcome = _modules.manager->importBlock(*sealed, Chain::ImportOrigin::local);
        if (!outcome)
            return core::fail(outcome.error(), "importing mined block");

        if (*outcome == Chain::ImportOutcome::imported)
            RSCoin_INFO("mined block {} ({} txs, head {})", sealed->header.height, sealed->transactions.size(), core::toHex(_modules.chain->headHash()));
        else
            RSCoin_WARN("mined block {} discarded (chain advanced during sealing)", sealed->header.height);
        return {};
    }

}

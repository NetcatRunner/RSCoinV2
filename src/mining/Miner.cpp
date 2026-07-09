#include "mining/Miner.hpp"

#include <chrono>
#include <utility>

#include "core/Hex.hpp"
#include "log/Logger.hpp"

namespace RSCoin::Mining {

    namespace {
        constexpr auto kRetryDelay = std::chrono::milliseconds{200};

        std::uint64_t nowSeconds() {
            return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        }
    }

    Miner::Miner(Dependencies dependencies, MiningConfig mining, Chain::BlockConfig block)
        : _deps(dependencies), _mining(std::move(mining)), _block(std::move(block)) {}

    Miner::~Miner() { stop(); }

    void Miner::start() {
        if (_thread.joinable())
            return;

        _stopSource = std::stop_source{};
        _thread = std::thread([this, cancel = _stopSource.get_token()] { miningLoop(cancel); });
        RSCoin_INFO("miner started (beneficiary {})", core::toHex(_mining.beneficiary));
    }

    void Miner::stop() {
        if (!_thread.joinable())
            return;

        _stopSource.request_stop();
        _thread.join();
        RSCoin_INFO("miner stopped");
    }

    void Miner::miningLoop(std::stop_token cancel) {
        while (!cancel.stop_requested()) {
            if (auto mined = mineOne(cancel); !mined && mined.error().code != core::ErrorCode::cancelled) {
                RSCoin_WARN("mining failed: {}", mined.error().describe());
                std::this_thread::sleep_for(kRetryDelay);
            }
        }
    }

    core::Result<void> Miner::mineOne(std::stop_token cancel) {
        Primitives::Block draft;
        draft.header.parentHash = _deps.chain.headHash();
        draft.header.height = _deps.chain.height() + 1;
        draft.header.timestamp = nowSeconds();
        draft.header.beneficiary = _mining.beneficiary;
        draft.transactions = _deps.mempool.take(_block.maxTransactions);

        if (auto prepared = _deps.consensus.prepare(draft.header, _deps.chain); !prepared)
            return core::fail(prepared.error(), "preparing block");

        auto preview = _deps.manager.ledger()->apply(draft);
        if (!preview)
            return core::fail(preview.error(), "executing draft");
        draft.header.stateRoot = (*preview)->stateRoot();

        auto sealed = _deps.consensus.seal(std::move(draft), _deps.chain, cancel);
        if (!sealed)
            return core::fail(sealed.error());

        auto outcome = _deps.manager.importBlock(*sealed, Chain::ImportOrigin::local);
        if (!outcome)
            return core::fail(outcome.error(), "importing mined block");

        if (*outcome == Chain::ImportOutcome::imported)
            RSCoin_INFO("mined block {} ({} txs, head {})", sealed->header.height, sealed->transactions.size(), core::toHex(_deps.chain.headHash()));
        else
            RSCoin_WARN("mined block {} discarded (chain advanced during sealing)", sealed->header.height);
        return {};
    }

}

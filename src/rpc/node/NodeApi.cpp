#include "rpc/node/NodeApi.hpp"

#include "primitives/Codec.hpp"

namespace RSCoin::Rpc {

    namespace {
        std::string_view toString(Mempool::AddOutcome outcome) {
            switch (outcome) {
                case Mempool::AddOutcome::added: return "added";
                case Mempool::AddOutcome::alreadyKnown: return "already-known";
                case Mempool::AddOutcome::rejected: return "rejected";
            }
            return "unknown";
        }
    }

    core::Result<SendReceipt> NodeApi::sendTransaction(const Primitives::Transaction& transaction) {
        const core::Hash256 hash = _services.hasher.hash(Primitives::encode(transaction));

        const auto outcome = _services.mempool.add(transaction);
        if (!outcome)
            return core::fail(outcome.error());
        return SendReceipt{hash, std::string(toString(*outcome))};
    }

    core::Result<AccountInfo> NodeApi::getAccount(const core::Address& address) {
        const auto view = _services.manager.ledger()->account(address);
        if (!view)
            return AccountInfo{};
        return AccountInfo{view->balance, view->nonce};
    }

    core::Result<NodeStatus> NodeApi::status() {
        return NodeStatus{
            _services.chain.height(),
            _services.chain.headHash(),
            _services.network.peerCount(),
            _services.mempool.size(),
        };
    }

}

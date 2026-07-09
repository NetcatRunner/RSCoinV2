#include "state/account/AccountStateMachine.hpp"

#include <cstdint>

namespace RSCoin::State {

    AccountStateMachine::AccountStateMachine(const Crypto::IHasher& hasher, const StateConfig& rules, const Chain::GenesisConfig& genesis)
        : _hasher(hasher), _blockReward(rules.blockReward) {
        for (const auto& allocation : genesis.allocations)
            _accounts[allocation.address] = Account{allocation.balance, 0};
    }

    core::Result<void> AccountStateMachine::validateAgainst(const Accounts& accounts, const Primitives::Transaction& transaction) {
        const auto sender = accounts.find(transaction.from);
        if (sender == accounts.end())
            return core::fail(core::ErrorCode::state, "unknown sender account");
        if (transaction.nonce != sender->second.nonce)
            return core::fail(core::ErrorCode::state, "bad nonce (expected " + std::to_string(sender->second.nonce) + ", got " + std::to_string(transaction.nonce) + ")");
        if (transaction.value.units > sender->second.balance.units)
            return core::fail(core::ErrorCode::state, "insufficient balance");
        return {};
    }

    core::Result<void> AccountStateMachine::validateTransaction(const Primitives::Transaction& transaction) const {
        return validateAgainst(_accounts, transaction);
    }

    core::Result<std::unique_ptr<IStateMachine>> AccountStateMachine::apply(const Primitives::Block& block) const {
        Accounts next = _accounts;

        for (const auto& transaction : block.transactions) {
            if (auto valid = validateAgainst(next, transaction); !valid)
                return core::fail(valid.error(), "invalid transaction in block");

            auto& sender = next[transaction.from];
            sender.balance.units -= transaction.value.units;
            sender.nonce += 1;
            next[transaction.to].balance.units += transaction.value.units;
        }

        next[block.header.beneficiary].balance.units += _blockReward.units;

        return std::unique_ptr<IStateMachine>(new AccountStateMachine(_hasher, _blockReward, std::move(next)));
    }

    core::Hash256 AccountStateMachine::stateRoot() const {
        core::Bytes encoded;
        encoded.reserve(_accounts.size() * 36);
        for (const auto& [address, account] : _accounts) {
            encoded.insert(encoded.end(), address.bytes.begin(), address.bytes.end());
            for (unsigned shift = 0; shift < 64; shift += 8)
                encoded.push_back(static_cast<std::byte>((account.balance.units >> shift) & 0xFF));
            for (unsigned shift = 0; shift < 64; shift += 8)
                encoded.push_back(static_cast<std::byte>((account.nonce >> shift) & 0xFF));
        }
        return _hasher.hash(encoded);
    }

}

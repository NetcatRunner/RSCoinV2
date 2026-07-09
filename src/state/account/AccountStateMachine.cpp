#include "state/account/AccountStateMachine.hpp"

#include <cstdint>

#include "primitives/Codec.hpp"

namespace RSCoin::State {

    AccountStateMachine::AccountStateMachine(Context context, const StateConfig& rules, const Chain::GenesisConfig& genesis)
        : _context(context), _blockReward(rules.blockReward) {
        for (const auto& allocation : genesis.allocations)
            _accounts[allocation.address] = Account{allocation.balance, 0};
    }

    core::Result<void> AccountStateMachine::validateAgainst(const Accounts& accounts, const Primitives::Transaction& transaction) const {
        const core::Hash256 digest = _context.hasher.hash(Primitives::encodeForSigning(transaction, _context.chainId));
        const auto signer = _context.signatures.authenticate(digest, transaction.signature);
        if (!signer)
            return core::fail(signer.error(), "transaction signature");
        if (*signer != transaction.from)
            return core::fail(core::ErrorCode::state, "signature does not match sender");

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

        return std::unique_ptr<IStateMachine>(new AccountStateMachine(_context, _blockReward, std::move(next)));
    }

    std::optional<AccountView> AccountStateMachine::account(const core::Address& address) const {
        const auto it = _accounts.find(address);
        if (it == _accounts.end())
            return std::nullopt;
        return AccountView{it->second.balance, it->second.nonce};
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
        return _context.hasher.hash(encoded);
    }

}

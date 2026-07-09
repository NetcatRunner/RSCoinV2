#pragma once

#include <vector>

#include "core/Result.hpp"
#include "core/Types.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Wallet {

    // Ce que l'utilisateur veut envoyer ; le wallet complète et signe.
    struct TransactionRequest {
        core::Address from;
        core::Address to;
        core::Amount value;
        std::uint64_t nonce{};
        core::Bytes payload;
    };

    // Gestion de clés et signature — les clés ne quittent JAMAIS le wallet :
    // le nœud ne reçoit que des transactions déjà signées.
    class IWallet {
    public:
        virtual ~IWallet() = default;

        virtual core::Result<core::Address> createAccount() = 0;
        virtual core::Result<std::vector<core::Address>> accounts() const = 0;
        virtual core::Result<Primitives::Transaction> signTransaction(const TransactionRequest& request) const = 0;
    };

}

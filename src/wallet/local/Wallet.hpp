#pragma once

#include "crypto/ICrypto.hpp"
#include "wallet/IWallet.hpp"
#include "wallet/keystore/Keystore.hpp"

namespace RSCoin::Wallet {

    class Wallet : public IWallet {
    public:
        Wallet(const Crypto::ICryptoProvider& crypto, Keystore keystore, std::uint64_t chainId)
            : _crypto(crypto), _keystore(std::move(keystore)), _chainId(chainId) {}

        core::Result<core::Address> createAccount() override;
        core::Result<std::vector<core::Address>> accounts() const override;
        core::Result<Primitives::Transaction> signTransaction(const TransactionRequest& request) const override;

    private:
        const Crypto::ICryptoProvider& _crypto;
        Keystore _keystore;
        std::uint64_t _chainId;
    };

}

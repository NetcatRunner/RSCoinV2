#include "wallet/local/Wallet.hpp"

#include "primitives/Codec.hpp"

namespace RSCoin::Wallet {

    core::Result<core::Address> Wallet::createAccount() {
        const auto keys = _crypto.signatures().generateKeyPair();
        if (!keys)
            return core::fail(keys.error());

        const auto address = _crypto.signatures().deriveAddress(keys->publicKey);
        if (!address)
            return core::fail(address.error());

        if (auto saved = _keystore.save(*address, *keys); !saved)
            return core::fail(saved.error());
        return *address;
    }

    core::Result<std::vector<core::Address>> Wallet::accounts() const {
        return _keystore.list();
    }

    core::Result<Primitives::Transaction> Wallet::signTransaction(const TransactionRequest& request) const {
        const auto keys = _keystore.load(request.from);
        if (!keys)
            return core::fail(keys.error());

        Primitives::Transaction transaction;
        transaction.from = request.from;
        transaction.to = request.to;
        transaction.value = request.value;
        transaction.nonce = request.nonce;
        transaction.payload = request.payload;

        const core::Hash256 digest = _crypto.hasher().hash(Primitives::encodeForSigning(transaction, _chainId));
        auto signature = _crypto.signatures().sign(digest, keys->privateKey);
        if (!signature)
            return core::fail(signature.error(), "signing transaction");
        transaction.signature = std::move(*signature);
        return transaction;
    }

}

#pragma once

#include <stop_token>

#include "core/Result.hpp"

namespace RSCoin::Wallet {

    class IWalletUi {
    public:
        virtual ~IWalletUi() = default;

        virtual core::Result<void> run(std::stop_token stop) = 0;
    };

}

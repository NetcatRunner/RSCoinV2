#pragma once

#include <string_view>

#include "network/INetwork.hpp"

namespace RSCoin::Protocol {

    class IProtocol : public Network::INetworkObserver {
    public:
        virtual std::string_view name() const noexcept = 0;

        virtual void tick() {}
    };

}

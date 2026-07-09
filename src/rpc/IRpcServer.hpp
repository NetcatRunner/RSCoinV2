#pragma once

#include "core/Result.hpp"

namespace RSCoin::Rpc {

    class IRpcServer {
    public:
        virtual ~IRpcServer() = default;

        virtual core::Result<void> start() = 0;
        virtual void stop() = 0;
    };

}

#pragma once

#include <cstdint>
#include <string>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Rpc {

    struct RpcConfig {
        static constexpr const char* kSection = "rpc";

        bool enabled{};
        std::string transport;
        std::string listenAddress;
        std::uint16_t port{};

        static core::Result<RpcConfig> from(const Config::Section& section);
    };

}

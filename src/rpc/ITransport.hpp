#pragma once

#include <string>
#include <string_view>

#include "core/Result.hpp"

namespace RSCoin::Rpc {

    class IRequestHandler {
    public:
        virtual ~IRequestHandler() = default;

        virtual std::string handle(std::string_view request) = 0;
        virtual std::string_view contentType() const noexcept = 0;
    };

    class IClientTransport {
    public:
        virtual ~IClientTransport() = default;

        virtual core::Result<std::string> roundTrip(const std::string& request, std::string_view contentType) = 0;
    };

}

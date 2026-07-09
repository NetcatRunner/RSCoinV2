#pragma once

#include <memory>

#include "rpc/INodeApi.hpp"
#include "rpc/ITransport.hpp"

namespace RSCoin::Rpc {

    class JsonRpcHandler : public IRequestHandler {
    public:
        JsonRpcHandler(std::unique_ptr<INodeApi> api) : _api(std::move(api)) {}

        std::string handle(std::string_view request) override;
        std::string_view contentType() const noexcept override { return "application/json"; }

    private:
        std::unique_ptr<INodeApi> _api;
    };

}

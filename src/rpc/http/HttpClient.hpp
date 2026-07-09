#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "rpc/ITransport.hpp"

namespace httplib { class Client; }

namespace RSCoin::Rpc {

    class HttpClient : public IClientTransport {
    public:
        HttpClient(const std::string& host, std::uint16_t port);
        ~HttpClient() override;

        core::Result<std::string> roundTrip(const std::string& request, std::string_view contentType) override;

    private:
        std::unique_ptr<httplib::Client> _client;
    };

}

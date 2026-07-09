#include "rpc/http/HttpClient.hpp"

#include <httplib.h>

namespace RSCoin::Rpc {

    HttpClient::HttpClient(const std::string& host, std::uint16_t port)
        : _client(std::make_unique<httplib::Client>(host, port)) {}

    HttpClient::~HttpClient() = default;

    core::Result<std::string> HttpClient::roundTrip(const std::string& request, std::string_view contentType) {
        const auto response = _client->Post("/", request, std::string(contentType));
        if (!response)
            return core::fail(core::ErrorCode::network, "rpc request failed: " + httplib::to_string(response.error()));
        if (response->status != 200)
            return core::fail(core::ErrorCode::protocol, "rpc server returned http status " + std::to_string(response->status));
        return response->body;
    }

}

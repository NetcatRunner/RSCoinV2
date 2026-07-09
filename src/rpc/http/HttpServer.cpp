#include "rpc/http/HttpServer.hpp"

#include <utility>

#include <httplib.h>

#include "log/Logger.hpp"

namespace RSCoin::Rpc {

    HttpServer::HttpServer(RpcConfig config, std::unique_ptr<IRequestHandler> handler)
        : _config(std::move(config)), _handler(std::move(handler)), _server(std::make_unique<httplib::Server>()) {
        _server->Post("/", [this](const httplib::Request& request, httplib::Response& response) {
            response.set_content(_handler->handle(request.body), std::string(_handler->contentType()));
        });
    }

    HttpServer::~HttpServer() { stop(); }

    core::Result<void> HttpServer::start() {
        if (_thread.joinable())
            return core::fail(core::ErrorCode::network, "rpc server already started");

        if (!_server->bind_to_port(_config.listenAddress, _config.port))
            return core::fail(core::ErrorCode::network,
                "cannot bind rpc server to " + _config.listenAddress + ":" + std::to_string(_config.port));

        _thread = std::thread([this] { _server->listen_after_bind(); });
        RSCoin_INFO("rpc (http) listening on {}:{}", _config.listenAddress, _config.port);
        return {};
    }

    void HttpServer::stop() {
        if (!_thread.joinable())
            return;
        _server->stop();
        _thread.join();
    }

}

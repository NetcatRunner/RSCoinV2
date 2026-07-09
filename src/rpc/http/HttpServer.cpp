#include "rpc/http/HttpRpcServer.hpp"

#include <utility>

#include <httplib.h>

#include "log/Logger.hpp"
#include "rpc/api/Envelope.hpp"

namespace RSCoin::Rpc {

    HttpRpcServer::HttpRpcServer(RpcConfig config, NodeServices services)
        : _config(std::move(config)), _methods(services), _server(std::make_unique<httplib::Server>()) {
        _server->Post("/", [this](const httplib::Request& request, httplib::Response& response) {
            response.set_content(handleRequest(_methods, request.body), "application/json");
        });
    }

    HttpRpcServer::~HttpRpcServer() { stop(); }

    core::Result<void> HttpRpcServer::start() {
        if (_thread.joinable())
            return core::fail(core::ErrorCode::network, "rpc server already started");

        // bind séparé du listen : l'échec de bind est rapporté ici, pas dans le thread.
        if (!_server->bind_to_port(_config.listenAddress, _config.port))
            return core::fail(core::ErrorCode::network,
                "cannot bind rpc server to " + _config.listenAddress + ":" + std::to_string(_config.port));

        _thread = std::thread([this] { _server->listen_after_bind(); });
        RSCoin_INFO("rpc (http) listening on {}:{}", _config.listenAddress, _config.port);
        return {};
    }

    void HttpRpcServer::stop() {
        if (!_thread.joinable())
            return;
        _server->stop();
        _thread.join();
    }

}

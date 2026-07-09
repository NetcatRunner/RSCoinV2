#pragma once

#include <memory>
#include <thread>

#include "rpc/IRpcServer.hpp"
#include "rpc/ITransport.hpp"
#include "rpc/RpcConfig.hpp"

namespace httplib { class Server; }

namespace RSCoin::Rpc {

    class HttpServer : public IRpcServer {
    public:
        HttpServer(RpcConfig config, std::unique_ptr<IRequestHandler> handler);
        ~HttpServer() override;

        core::Result<void> start() override;
        void stop() override;

    private:
        RpcConfig _config;
        std::unique_ptr<IRequestHandler> _handler;

        std::unique_ptr<httplib::Server> _server;
        std::thread _thread;
    };

}

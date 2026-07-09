#pragma once

#include <memory>
#include <thread>

#include "rpc/IRpcServer.hpp"
#include "rpc/RpcConfig.hpp"
#include "rpc/api/Methods.hpp"

namespace httplib { class Server; }

namespace RSCoin::Rpc {

    // JSON-RPC 2.0 sur HTTP (convention Ethereum : POST / avec le corps JSON) —
    // testable avec curl. Bâti sur cpp-httplib, qui gère son pool de threads.
    class HttpRpcServer : public IRpcServer {
    public:
        HttpRpcServer(RpcConfig config, NodeServices services);
        ~HttpRpcServer() override;

        core::Result<void> start() override;
        void stop() override;

    private:
        RpcConfig _config;
        Methods _methods;

        std::unique_ptr<httplib::Server> _server;
        std::thread _thread;
    };

}

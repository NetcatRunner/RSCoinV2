#include "Node.hpp"

#include <utility>

#include <RST/time/FrameTimer.hpp>

#include "core/Hex.hpp"
#include "log/Logger.hpp"

namespace RSCoin::Node {

    Node::Node(Modules modules): _modules(std::move(modules)) {}

    core::Result<void> Node::run(std::stop_token stop) {
        RSCoin_INFO("chain head: height {} ({})", _modules.chain->height(), core::toHex(_modules.chain->headHash()));

        auto started = _modules.network->start(*_modules.protocol);
        if (!started)
            return started;

        if (_modules.miner)
            _modules.miner->start();

        auto frameTimer = RST::Time::FrameTimer(10);
        while (!stop.stop_requested()) {
            _modules.protocol->tick();
            frameTimer.tick();
        }

        if (_modules.miner)
            _modules.miner->stop();
        _modules.network->stop();
        return {};
    }

}

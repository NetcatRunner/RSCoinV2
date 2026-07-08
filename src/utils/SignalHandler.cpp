#include "SignalHandler.hpp"

#include <csignal>

namespace RSCoin::Utils {

    void setupHandlers() {
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
    }

    std::stop_source& stopSource() {
        static std::stop_source source;
        return source;
    }

    void handleSignal(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            stopSource().request_stop();
        }
    }
}

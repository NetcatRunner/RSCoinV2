#pragma once

#include <stop_token>

namespace RSCoin::Utils {

    void setupHandlers();

    std::stop_source& stopSource();
    void handleSignal(int signal);
}

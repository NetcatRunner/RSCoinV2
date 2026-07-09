#pragma once

#include <mutex>

#include "network/INetwork.hpp"
#include "network/tcp/Socket.hpp"

namespace RSCoin::Network {

    struct Peer {
        PeerId id;
        Socket socket;
        std::mutex writeMutex;
    };

}

#pragma once

#include <mutex>

#include "network/INetwork.hpp"
#include "network/utils/Socket.hpp"

namespace RSCoin::Network {

    struct Peer {
        PeerId id;
        Socket socket;
        std::mutex writeMutex;
    };

}

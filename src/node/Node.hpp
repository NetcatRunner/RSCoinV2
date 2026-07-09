#pragma once

#include "NodeModules.hpp"
#include <stop_token>

namespace RSCoin::Node {

    class Node {
    public:
        Node(Modules modules);

        core::Result<void> run(std::stop_token stop);

    private:
        Modules _modules;
    };

}

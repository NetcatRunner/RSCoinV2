#pragma once

#include "config/Store.hpp"
#include "core/Result.hpp"
#include "node/Node.hpp"

namespace RSCoin::Node {

    core::Result<Node> makeNode(const Config::Store& network, const Config::Store& node);

}

#pragma once

#include <cstdint>
#include <string>

#include "core/Types.hpp"

namespace RSCoin::Rpc {

    // Les réponses de l'API du nœud, en types du domaine : le JSON n'est
    // qu'un format de fil, confiné à rpc/api/ et rpc/http/.
    struct AccountInfo {
        core::Amount balance;
        std::uint64_t nonce{};
    };

    struct NodeStatus {
        std::uint64_t height{};
        core::Hash256 headHash;
        std::size_t peers{};
        std::size_t mempoolSize{};
    };

    struct SendReceipt {
        core::Hash256 transactionHash;
        std::string outcome;
    };

}

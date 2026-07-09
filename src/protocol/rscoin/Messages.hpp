#pragma once

#include <cstdint>
#include <vector>

#include "core/Result.hpp"
#include "core/Types.hpp"
#include "primitives/Block.hpp"
#include "protocol/rscoin/Topics.hpp"

namespace RSCoin::Protocol {

    struct StatusMessage {
        static constexpr std::uint16_t kTopic = Topics::kStatus;

        std::uint32_t version{};
        std::uint64_t chainId{};
        std::uint64_t height{};
        core::Hash256 headHash;

        core::Bytes encode() const;
        static core::Result<StatusMessage> decode(core::BytesView payload);
    };

    struct PingMessage {
        static constexpr std::uint16_t kTopic = Topics::kPing;

        std::uint64_t nonce{};

        core::Bytes encode() const;
        static core::Result<PingMessage> decode(core::BytesView payload);
    };

    struct PongMessage {
        static constexpr std::uint16_t kTopic = Topics::kPong;

        std::uint64_t nonce{};

        core::Bytes encode() const;
        static core::Result<PongMessage> decode(core::BytesView payload);
    };

    struct NewBlockMessage {
        static constexpr std::uint16_t kTopic = Topics::kNewBlock;

        Primitives::Block block;

        core::Bytes encode() const;
        static core::Result<NewBlockMessage> decode(core::BytesView payload);
    };

    struct GetBlocksMessage {
        static constexpr std::uint16_t kTopic = Topics::kGetBlocks;

        std::uint64_t fromHeight{};
        std::uint32_t maxCount{};

        core::Bytes encode() const;
        static core::Result<GetBlocksMessage> decode(core::BytesView payload);
    };

    struct BlocksMessage {
        static constexpr std::uint16_t kTopic = Topics::kBlocks;

        std::vector<Primitives::Block> blocks;

        core::Bytes encode() const;
        static core::Result<BlocksMessage> decode(core::BytesView payload);
    };

    struct NewTransactionMessage {
        static constexpr std::uint16_t kTopic = Topics::kNewTransaction;

        Primitives::Transaction transaction;

        core::Bytes encode() const;
        static core::Result<NewTransactionMessage> decode(core::BytesView payload);
    };

}

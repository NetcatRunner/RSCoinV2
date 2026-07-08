#include "protocol/rscoin/Messages.hpp"

#include "primitives/Codec.hpp"
#include "protocol/Codec.hpp"

namespace RSCoin::Protocol {

    namespace {
        auto malformed(const char* what) {
            return core::fail(core::ErrorCode::protocol, std::string("malformed ") + what + " message");
        }
    }

    core::Bytes StatusMessage::encode() const {
        return Writer{}.write(version).write(chainId).write(height).writeFixed(headHash).take();
    }

    core::Result<StatusMessage> StatusMessage::decode(core::BytesView payload) {
        Reader reader(payload);
        const auto version = reader.read<std::uint32_t>();
        const auto chainId = reader.read<std::uint64_t>();
        const auto height = reader.read<std::uint64_t>();
        const auto headHash = reader.readFixed<32>();
        if (!version || !chainId || !height || !headHash || !reader.finished())
            return malformed("status");
        return StatusMessage{*version, *chainId, *height, *headHash};
    }

    core::Bytes PingMessage::encode() const {
        return Writer{}.write(nonce).take();
    }

    core::Result<PingMessage> PingMessage::decode(core::BytesView payload) {
        Reader reader(payload);
        const auto nonce = reader.read<std::uint64_t>();
        if (!nonce || !reader.finished())
            return malformed("ping");
        return PingMessage{*nonce};
    }

    core::Bytes PongMessage::encode() const {
        return Writer{}.write(nonce).take();
    }

    core::Result<PongMessage> PongMessage::decode(core::BytesView payload) {
        Reader reader(payload);
        const auto nonce = reader.read<std::uint64_t>();
        if (!nonce || !reader.finished())
            return malformed("pong");
        return PongMessage{*nonce};
    }

    core::Bytes NewBlockMessage::encode() const {
        return Writer{}.writeBytes(Primitives::encode(block)).take();
    }

    core::Result<NewBlockMessage> NewBlockMessage::decode(core::BytesView payload) {
        Reader reader(payload);
        const auto raw = reader.readBytes();
        if (!raw || !reader.finished())
            return malformed("new-block");

        auto block = Primitives::decodeBlock(*raw);
        if (!block)
            return core::fail(block.error(), "malformed new-block message");
        return NewBlockMessage{std::move(*block)};
    }

    core::Bytes GetBlocksMessage::encode() const {
        return Writer{}.write(fromHeight).write(maxCount).take();
    }

    core::Result<GetBlocksMessage> GetBlocksMessage::decode(core::BytesView payload) {
        Reader reader(payload);
        const auto fromHeight = reader.read<std::uint64_t>();
        const auto maxCount = reader.read<std::uint32_t>();
        if (!fromHeight || !maxCount || !reader.finished())
            return malformed("get-blocks");
        return GetBlocksMessage{*fromHeight, *maxCount};
    }

    core::Bytes BlocksMessage::encode() const {
        Writer writer;
        writer.write(static_cast<std::uint32_t>(blocks.size()));
        for (const auto& block : blocks)
            writer.writeBytes(Primitives::encode(block));
        return writer.take();
    }

    core::Result<BlocksMessage> BlocksMessage::decode(core::BytesView payload) {
        Reader reader(payload);
        const auto count = reader.read<std::uint32_t>();
        if (!count)
            return malformed("blocks");

        BlocksMessage message;
        message.blocks.reserve(*count);
        for (std::uint32_t i = 0; i < *count; ++i) {
            const auto raw = reader.readBytes();
            if (!raw)
                return malformed("blocks");
            auto block = Primitives::decodeBlock(*raw);
            if (!block)
                return core::fail(block.error(), "malformed blocks message");
            message.blocks.push_back(std::move(*block));
        }
        if (!reader.finished())
            return malformed("blocks");
        return message;
    }

}

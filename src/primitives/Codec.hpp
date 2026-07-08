#pragma once

#include "core/Result.hpp"
#include "core/Types.hpp"
#include "primitives/Block.hpp"

namespace RSCoin::Primitives {

    core::Bytes encode(const BlockHeader& header);
    core::Bytes encode(const Transaction& transaction);
    core::Bytes encode(const Block& block);

    core::Result<BlockHeader> decodeHeader(core::BytesView data);
    core::Result<Transaction> decodeTransaction(core::BytesView data);
    core::Result<Block> decodeBlock(core::BytesView data);

}

#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "core/Types.hpp"
#include "primitives/Transaction.hpp"

namespace RSCoin::Primitives {

    struct HeaderExtension {
        std::uint16_t tag{};
        core::Bytes payload;
        auto operator<=>(const HeaderExtension&) const = default;
    };

    struct BlockHeader {
        static constexpr std::uint32_t kCurrentVersion = 1;

        std::uint32_t version{kCurrentVersion};
        core::Hash256 parentHash;
        core::Hash256 stateRoot;
        core::Hash256 transactionsRoot;
        std::uint64_t height{};
        std::uint64_t timestamp{};
        core::Address beneficiary;
        core::Bytes extraData;
        core::Bytes consensusSeal;
        std::vector<HeaderExtension> extensions;
    };

    struct Block {
        BlockHeader header;
        std::vector<Transaction> transactions;
    };

    inline std::optional<core::BytesView> findExtension(const BlockHeader& header, std::uint16_t tag) {
        for (const auto& extension : header.extensions) {
            if (extension.tag == tag) {
                return core::BytesView{extension.payload};
            }
        }
        return std::nullopt;
    }

    inline void setExtension(BlockHeader& header, std::uint16_t tag, core::Bytes payload) {
        for (auto& extension : header.extensions) {
            if (extension.tag == tag) {
                extension.payload = std::move(payload);
                return;
            }
        }
        header.extensions.push_back(HeaderExtension{tag, std::move(payload)});
    }

}

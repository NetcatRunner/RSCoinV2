#pragma once

#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Storage {

    struct WriteBatch {
        struct Put {
            core::Bytes key;
            core::Bytes value;
        };
        struct Erase {
            core::Bytes key;
        };

        std::vector<std::variant<Put, Erase>> operations;

        WriteBatch& put(core::Bytes key, core::Bytes value) {
            operations.emplace_back(Put{std::move(key), std::move(value)});
            return *this;
        }
        WriteBatch& erase(core::Bytes key) {
            operations.emplace_back(Erase{std::move(key)});
            return *this;
        }
    };

    class IKeyValueStore {
    public:
        virtual ~IKeyValueStore() = default;

        virtual core::Result<std::optional<core::Bytes>> get(core::BytesView key) const = 0;
        virtual core::Result<void> put(core::BytesView key, core::BytesView value) = 0;
        virtual core::Result<void> erase(core::BytesView key) = 0;

        virtual core::Result<void> apply(WriteBatch batch) = 0;
    };

}

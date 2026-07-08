#pragma once

#include <string>
#include <string_view>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::core {

    namespace detail {
        inline int nibble(char c) noexcept {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }
    }

    inline Result<Bytes> fromHex(std::string_view hex) {
        if (hex.starts_with("0x") || hex.starts_with("0X"))
            hex.remove_prefix(2);
        if (hex.size() % 2 != 0)
            return fail(ErrorCode::validation, "hex string has odd length");

        Bytes out;
        out.reserve(hex.size() / 2);
        for (std::size_t i = 0; i < hex.size(); i += 2) {
            const int hi = detail::nibble(hex[i]);
            const int lo = detail::nibble(hex[i + 1]);
            if (hi < 0 || lo < 0)
                return fail(ErrorCode::validation, "invalid hex character");
            out.push_back(static_cast<std::byte>((hi << 4) | lo));
        }
        return out;
    }

    template <std::size_t N>
    Result<FixedBytes<N>> fixedFromHex(std::string_view hex) {
        auto bytes = fromHex(hex);
        if (!bytes)
            return fail(bytes.error());
        if (bytes->size() != N)
            return fail(ErrorCode::validation, "expected " + std::to_string(N) + " bytes, got " + std::to_string(bytes->size()));

        FixedBytes<N> out;
        for (std::size_t i = 0; i < N; ++i)
            out.bytes[i] = (*bytes)[i];
        return out;
    }

    inline std::string toHex(BytesView data) {
        static constexpr char digits[] = "0123456789abcdef";
        std::string out;
        out.reserve(2 + data.size() * 2);
        out += "0x";
        for (const std::byte b : data) {
            out.push_back(digits[std::to_integer<unsigned>(b) >> 4]);
            out.push_back(digits[std::to_integer<unsigned>(b) & 0x0F]);
        }
        return out;
    }

    template <std::size_t N>
    std::string toHex(const FixedBytes<N>& data) {
        return toHex(BytesView{data.bytes});
    }

}

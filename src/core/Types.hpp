#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace RSCoin::core {

    using Bytes = std::vector<std::byte>;
    using BytesView = std::span<const std::byte>;

    template <std::size_t N>
    struct FixedBytes {
        std::array<std::byte, N> bytes{};

        static constexpr std::size_t size() noexcept { return N; }
        auto operator<=>(const FixedBytes&) const = default;
    };

    using Hash256 = FixedBytes<32>;
    using Address = FixedBytes<20>;

    struct PublicKey {
        Bytes data;
        auto operator<=>(const PublicKey&) const = default;
    };

    struct PrivateKey {
        Bytes data;
    };

    struct Signature {
        Bytes data;
        auto operator<=>(const Signature&) const = default;
    };

    struct KeyPair {
        PrivateKey privateKey;
        PublicKey publicKey;
    };

    struct Amount {
        std::uint64_t units{};
        auto operator<=>(const Amount&) const = default;
    };

}

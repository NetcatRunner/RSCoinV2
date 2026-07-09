#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace RSCoin::Crypto {

    class Sha256 {
    public:
        static constexpr std::size_t SIZE = 32;
        using Digest = std::array<std::uint8_t, SIZE>;

        Sha256();

        Sha256& reset() noexcept;

        Sha256& update(const std::uint8_t* data, std::size_t length);
        Sha256& update(std::string_view data);
        Sha256& update(const std::vector<std::uint8_t>& data);
        Sha256& update(std::span<const std::uint8_t> data);

        Digest digest();

        std::string hexDigest();

        static Digest hashRaw(std::string_view data);
        static Digest hashRaw(const std::uint8_t* data, std::size_t len);
        static Digest hashRaw(const std::vector<std::uint8_t>& data);
        static Digest hashRaw(std::span<const std::uint8_t> data);
        static std::string hash(std::string_view data);

        static Digest hashStream(std::istream& in);
        static Digest hashFile(std::string_view path);
        static std::string hashFileHex(std::string_view path);

        static Digest hmacRaw(std::string_view key, std::string_view message);
        static std::string hmac(std::string_view key, std::string_view message);

        static std::string toHex(const Digest& digest);
        static Digest fromHex(std::string_view hex);

        static bool equal(const Digest& a, const Digest& b) noexcept;
    private:
        static constexpr std::array<std::uint32_t, 64> K = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08,  0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        std::uint8_t _data[64]{};
        std::uint32_t _blocklen{0};
        std::uint64_t _bitlen{0};
        std::uint32_t _state[8]{};

        static constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n) noexcept;
        static constexpr std::uint32_t choose(std::uint32_t e, std::uint32_t f, std::uint32_t g) noexcept;
        static constexpr std::uint32_t majority(std::uint32_t a, std::uint32_t b, std::uint32_t c) noexcept;
        static constexpr std::uint32_t sig0(std::uint32_t x) noexcept;
        static constexpr std::uint32_t sig1(std::uint32_t x) noexcept;

        void transform() noexcept;
        void pad();
        void revert(Digest& hash) const noexcept;
    };
}

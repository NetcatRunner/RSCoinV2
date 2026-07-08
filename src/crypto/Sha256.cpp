#include "crypto/Sha256.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace RSCoin::Crypto {

    constexpr std::uint32_t Sha256::rotr(std::uint32_t x, std::uint32_t n) noexcept {
        return (x >> n) | (x << (32u - n));
    }

    constexpr std::uint32_t Sha256::choose(std::uint32_t e, std::uint32_t f, std::uint32_t g) noexcept {
        return (e & f) ^ (~e & g);
    }

    constexpr std::uint32_t Sha256::majority(std::uint32_t a, std::uint32_t b, std::uint32_t c) noexcept {
        return (a & b) ^ (a & c) ^ (b & c);
    }

    constexpr std::uint32_t Sha256::sig0(std::uint32_t x) noexcept {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    constexpr std::uint32_t Sha256::sig1(std::uint32_t x) noexcept {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    Sha256::Sha256() {
        reset();
    }

    Sha256& Sha256::reset() noexcept {
        m_blocklen = 0;
        m_bitlen   = 0;
        m_state[0] = 0x6a09e667u;
        m_state[1] = 0xbb67ae85u;
        m_state[2] = 0x3c6ef372u;
        m_state[3] = 0xa54ff53au;
        m_state[4] = 0x510e527fu;
        m_state[5] = 0x9b05688cu;
        m_state[6] = 0x1f83d9abu;
        m_state[7] = 0x5be0cd19u;
        return *this;
    }

    Sha256& Sha256::update(const std::uint8_t* data, std::size_t length) {
        for (std::size_t i = 0; i < length; ++i) {
            m_data[m_blocklen++] = data[i];
            if (m_blocklen == 64) {
                transform();
                m_bitlen  += 512;
                m_blocklen = 0;
            }
        }
        return *this;
    }

    Sha256& Sha256::update(std::string_view data) {
        return update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
    }

    Sha256& Sha256::update(const std::vector<std::uint8_t>& data) {
        return update(data.data(), data.size());
    }

    Sha256& Sha256::update(std::span<const std::uint8_t> data) {
        return update(data.data(), data.size());
    }

    Sha256::Digest Sha256::digest() {
        Digest hash;
        pad();
        revert(hash);
        reset();
        return hash;
    }

    std::string Sha256::hexDigest() {
        return toHex(digest());
    }

    void Sha256::transform() noexcept {
        std::uint32_t m[64];

        for (std::uint8_t i = 0, j = 0; i < 16; ++i, j += 4) {
            m[i] = (static_cast<std::uint32_t>(m_data[j    ]) << 24)
                | (static_cast<std::uint32_t>(m_data[j + 1]) << 16)
                | (static_cast<std::uint32_t>(m_data[j + 2]) <<  8)
                |  static_cast<std::uint32_t>(m_data[j + 3]);
        }
        for (std::uint8_t k = 16; k < 64; ++k) {
            m[k] = sig1(m[k - 2]) + m[k - 7] + sig0(m[k - 15]) + m[k - 16];
        }

        std::uint32_t a = m_state[0], b = m_state[1],
                    c = m_state[2], d = m_state[3],
                    e = m_state[4], f = m_state[5],
                    g = m_state[6], h = m_state[7];

        for (std::uint8_t i = 0; i < 64; ++i) {
            const std::uint32_t S1  = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch  = choose(e, f, g);
            const std::uint32_t t1  = h + S1 + ch + K[i] + m[i];
            const std::uint32_t S0  = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = majority(a, b, c);
            const std::uint32_t t2  = S0 + maj;

            h = g; g = f; f = e;
            e = d + t1;
            d = c; c = b; b = a;
            a = t1 + t2;
        }

        m_state[0] += a; m_state[1] += b;
        m_state[2] += c; m_state[3] += d;
        m_state[4] += e; m_state[5] += f;
        m_state[6] += g; m_state[7] += h;
    }

    void Sha256::pad() {
        std::uint64_t i = m_blocklen;
        std::uint8_t end = (m_blocklen < 56) ? 56 : 64;

        m_data[i++] = 0x80;
        while (i < end) m_data[i++] = 0x00;

        if (m_blocklen >= 56) {
            transform();
            std::memset(m_data, 0, 56);
        }

        m_bitlen += m_blocklen * 8;
        m_data[63] = static_cast<std::uint8_t>(m_bitlen);
        m_data[62] = static_cast<std::uint8_t>(m_bitlen >>  8);
        m_data[61] = static_cast<std::uint8_t>(m_bitlen >> 16);
        m_data[60] = static_cast<std::uint8_t>(m_bitlen >> 24);
        m_data[59] = static_cast<std::uint8_t>(m_bitlen >> 32);
        m_data[58] = static_cast<std::uint8_t>(m_bitlen >> 40);
        m_data[57] = static_cast<std::uint8_t>(m_bitlen >> 48);
        m_data[56] = static_cast<std::uint8_t>(m_bitlen >> 56);
        transform();
    }

    void Sha256::revert(Digest& hash) const noexcept {
        for (std::uint8_t i = 0; i < 4; ++i) {
            for (std::uint8_t j = 0; j < 8; ++j) {
                hash[i + j * 4] = static_cast<std::uint8_t>(
                    (m_state[j] >> (24 - i * 8)) & 0xff
                );
            }
        }
    }

    Sha256::Digest Sha256::hashRaw(std::string_view data) {
        return Sha256{}.update(data).digest();
    }

    Sha256::Digest Sha256::hashRaw(const std::uint8_t* data, std::size_t len) {
        return Sha256{}.update(data, len).digest();
    }

    Sha256::Digest Sha256::hashRaw(const std::vector<std::uint8_t>& data) {
        return Sha256{}.update(data).digest();
    }

    Sha256::Digest Sha256::hashRaw(std::span<const std::uint8_t> data) {
        return Sha256{}.update(data).digest();
    }

    std::string Sha256::hash(std::string_view data) {
        return toHex(hashRaw(data));
    }

    Sha256::Digest Sha256::hashStream(std::istream& in) {
        Sha256 h;
        std::array<char, 8192> buf{};
        while (in.read(buf.data(), buf.size()) || in.gcount() > 0) {
            h.update(reinterpret_cast<const std::uint8_t*>(buf.data()), static_cast<std::size_t>(in.gcount()));
        }
        return h.digest();
    }

    Sha256::Digest Sha256::hashFile(std::string_view path) {
        std::ifstream f{std::string(path), std::ios::binary};
        if (!f)
            throw std::runtime_error("Sha256::hashFile: cannot open '" + std::string(path) + "'");
        return hashStream(f);
    }

    std::string Sha256::hashFileHex(std::string_view path) {
        return toHex(hashFile(path));
    }

    Sha256::Digest Sha256::hmacRaw(std::string_view key, std::string_view message) {
        constexpr std::size_t BLOCK = 64;

        std::array<std::uint8_t, BLOCK> k{};
        if (key.size() > BLOCK) {
            auto dk = hashRaw(key);
            std::copy(dk.begin(), dk.end(), k.begin());
        } else {
            std::copy(key.begin(), key.end(), k.begin());
        }

        std::array<std::uint8_t, BLOCK> ipad{}, opad{};
        for (std::size_t i = 0; i < BLOCK; ++i) {
            ipad[i] = k[i] ^ 0x36u;
            opad[i] = k[i] ^ 0x5cu;
        }

        Sha256 inner;
        inner.update(ipad).update(message);
        Digest inner_hash = inner.digest();

        Sha256 outer;
        outer.update(opad).update(inner_hash);
        return outer.digest();
    }

    std::string Sha256::hmac(std::string_view key, std::string_view message) {
        return toHex(hmacRaw(key, message));
    }

    std::string Sha256::toHex(const Digest& digest) {
        std::string out;
        out.reserve(64);
        constexpr char hex[] = "0123456789abcdef";
        for (std::uint8_t b : digest) {
            out.push_back(hex[b >> 4]);
            out.push_back(hex[b & 0x0f]);
        }
        return out;
    }

    Sha256::Digest Sha256::fromHex(std::string_view hex) {
        if (hex.size() != 64)
            throw std::invalid_argument("Sha256::fromHex: expected 64 hex chars");
        Digest d{};
        auto val = [](char c) -> std::uint8_t {
            if (c >= '0' && c <= '9')
                return static_cast<std::uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f')
                return static_cast<std::uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F')
                return static_cast<std::uint8_t>(c - 'A' + 10);
            throw std::invalid_argument("Sha256::fromHex: invalid hex character");
        };
        for (std::size_t i = 0; i < 32; ++i)
            d[i] = static_cast<std::uint8_t>((val(hex[i * 2]) << 4) | val(hex[i * 2 + 1]));
        return d;
    }

    bool Sha256::equal(const Digest& a, const Digest& b) noexcept {
        std::uint8_t diff = 0;
        for (std::size_t i = 0; i < a.size(); ++i)
            diff |= a[i] ^ b[i];
        return diff == 0;
    }

}

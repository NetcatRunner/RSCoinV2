#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Network {

    // Socket TCP POSIX en RAII — boîte à outils du module réseau,
    // utilisée par le transport P2P (network/tcp).
    class Socket {
    public:
        Socket() = default;
        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;
        ~Socket();

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        static core::Result<Socket> listen(const std::string& host, std::uint16_t port);
        static core::Result<Socket> connect(const std::string& host, std::uint16_t port);

        core::Result<Socket> accept() const;

        core::Result<std::size_t> receiveSome(std::span<std::byte> buffer) const;
        core::Result<void> receiveExact(std::span<std::byte> buffer) const;
        core::Result<void> sendAll(core::BytesView data) const;

        void shutdown() noexcept;

        bool isValid() const noexcept { return _fd >= 0; }
        std::string remoteAddress() const;

    private:
        static constexpr int kListenBacklog = 16;
        Socket(int fd) : _fd(fd) {}
        void close() noexcept;

        int _fd{-1};
    };

}

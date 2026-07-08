#pragma once

#include <cstddef>
#include <span>
#include <string>

#include "core/Result.hpp"
#include "core/Types.hpp"
#include "network/INetwork.hpp"

namespace RSCoin::Network {

    class Socket {
    public:
        Socket() = default;
        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;
        ~Socket();

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        static core::Result<Socket> listen(const Endpoint& endpoint);
        static core::Result<Socket> connect(const Endpoint& endpoint);

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

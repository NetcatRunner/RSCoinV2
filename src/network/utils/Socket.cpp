#include "network/utils/Socket.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <utility>

namespace RSCoin::Network {

    namespace {
        auto sysFail(std::string what) {
            return core::fail(core::ErrorCode::network, std::move(what) + ": " + std::strerror(errno));
        }

        std::string toString(const std::string& host, std::uint16_t port) {
            return host + ":" + std::to_string(port);
        }

        core::Result<sockaddr_in> resolve(const std::string& host, std::uint16_t port) {
            addrinfo hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            addrinfo* results = nullptr;
            const int status = ::getaddrinfo(host.c_str(), nullptr, &hints, &results);
            if (status != 0 || results == nullptr)
                return core::fail(core::ErrorCode::network, "cannot resolve host '" + host + "': " + ::gai_strerror(status));

            sockaddr_in address{};
            std::memcpy(&address, results->ai_addr, sizeof(address));
            address.sin_port = htons(port);
            ::freeaddrinfo(results);
            return address;
        }
    }

    Socket::Socket(Socket&& other) noexcept : _fd(std::exchange(other._fd, -1)) {}

    Socket& Socket::operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            _fd = std::exchange(other._fd, -1);
        }
        return *this;
    }

    Socket::~Socket() { close(); }

    void Socket::close() noexcept {
        if (_fd >= 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    void Socket::shutdown() noexcept {
        if (_fd >= 0)
            ::shutdown(_fd, SHUT_RDWR);
    }

    core::Result<Socket> Socket::listen(const std::string& host, std::uint16_t port) {
        auto address = resolve(host, port);
        if (!address)
            return core::fail(address.error());

        Socket socket{::socket(AF_INET, SOCK_STREAM, 0)};
        if (!socket.isValid())
            return sysFail("socket() failed");

        const int enable = 1;
        ::setsockopt(socket._fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

        if (::bind(socket._fd, reinterpret_cast<const sockaddr*>(&*address), sizeof(*address)) < 0)
            return sysFail("bind() failed on " + toString(host, port));
        if (::listen(socket._fd, kListenBacklog) < 0)
            return sysFail("listen() failed on " + toString(host, port));

        return socket;
    }

    core::Result<Socket> Socket::connect(const std::string& host, std::uint16_t port) {
        auto address = resolve(host, port);
        if (!address)
            return core::fail(address.error());

        Socket socket{::socket(AF_INET, SOCK_STREAM, 0)};
        if (!socket.isValid())
            return sysFail("socket() failed");

        if (::connect(socket._fd, reinterpret_cast<const sockaddr*>(&*address), sizeof(*address)) < 0)
            return sysFail("connect() failed to " + toString(host, port));

        return socket;
    }

    core::Result<Socket> Socket::accept() const {
        const int fd = ::accept(_fd, nullptr, nullptr);
        if (fd < 0)
            return sysFail("accept() failed");
        return Socket{fd};
    }

    core::Result<std::size_t> Socket::receiveSome(std::span<std::byte> buffer) const {
        while (true) {
            const ssize_t received = ::recv(_fd, buffer.data(), buffer.size(), 0);
            if (received >= 0)
                return static_cast<std::size_t>(received);
            if (errno != EINTR)
                return sysFail("recv() failed");
        }
    }

    core::Result<void> Socket::receiveExact(std::span<std::byte> buffer) const {
        std::size_t total = 0;
        while (total < buffer.size()) {
            auto received = receiveSome(buffer.subspan(total));
            if (!received)
                return core::fail(received.error());
            if (*received == 0)
                return core::fail(core::ErrorCode::network, "connection closed by peer");
            total += *received;
        }
        return {};
    }

    core::Result<void> Socket::sendAll(core::BytesView data) const {
        std::size_t total = 0;
        while (total < data.size()) {
            const ssize_t sent = ::send(_fd, data.data() + total, data.size() - total, MSG_NOSIGNAL);
            if (sent < 0) {
                if (errno == EINTR)
                    continue;
                return sysFail("send() failed");
            }
            total += static_cast<std::size_t>(sent);
        }
        return {};
    }

    std::string Socket::remoteAddress() const {
        sockaddr_in address{};
        socklen_t length = sizeof(address);
        if (::getpeername(_fd, reinterpret_cast<sockaddr*>(&address), &length) < 0)
            return "?";

        char host[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, &address.sin_addr, host, sizeof(host));
        return std::string(host) + ":" + std::to_string(ntohs(address.sin_port));
    }

}

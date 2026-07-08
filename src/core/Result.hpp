#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace RSCoin::core {

    enum class ErrorCode {
        config,
        crypto,
        storage,
        network,
        protocol,
        consensus,
        state,
        validation,
        notFound,
        cancelled,
        notImplemented,
    };

    constexpr std::string_view toString(ErrorCode code) noexcept {
        switch (code) {
            case ErrorCode::config: return "config";
            case ErrorCode::crypto: return "crypto";
            case ErrorCode::storage: return "storage";
            case ErrorCode::network: return "network";
            case ErrorCode::protocol: return "protocol";
            case ErrorCode::consensus: return "consensus";
            case ErrorCode::state: return "state";
            case ErrorCode::validation: return "validation";
            case ErrorCode::notFound: return "not-found";
            case ErrorCode::cancelled: return "cancelled";
            case ErrorCode::notImplemented: return "not-implemented";
        }
        return "unknown";
    }

    struct Error {
        ErrorCode code{};
        std::string message;

        std::string describe() const { return "[" + std::string(toString(code)) + "] " + message; }
    };

    template <typename T = void>
    using Result = std::expected<T, Error>;

    inline auto fail(ErrorCode code, std::string message) {
        return std::unexpected(Error{code, std::move(message)});
    }

    inline auto fail(Error error) {
        return std::unexpected(std::move(error));
    }

    inline auto fail(Error error, std::string_view context) {
        error.message = std::string(context) + ": " + error.message;
        return std::unexpected(std::move(error));
    }

}

#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

#include "protocol/Message.hpp"

namespace RSCoin::Protocol {

    class Dispatcher {
    public:
        template <Message T>
        void on(std::function<void(const Network::PeerId&, const T&)> handler) {
            _handlers[T::kTopic] = [handler = std::move(handler)](const Network::PeerId& from, core::BytesView payload) -> core::Result<void> {
                auto message = T::decode(payload);
                if (!message)
                    return core::fail(message.error());
                handler(from, *message);
                return {};
            };
        }

        core::Result<bool> dispatch(const Network::PeerId& from, const Network::NetMessage& message) const {
            const auto it = _handlers.find(message.topic);
            if (it == _handlers.end())
                return false;

            if (auto handled = it->second(from, message.payload); !handled)
                return core::fail(handled.error());
            return true;
        }

    private:
        using Handler = std::function<core::Result<void>(const Network::PeerId&, core::BytesView)>;
        std::unordered_map<std::uint16_t, Handler> _handlers;
    };

}

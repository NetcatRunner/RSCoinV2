#include "config/Section.hpp"

#include <limits>

#include <nlohmann/json.hpp>

#include "core/Hex.hpp"

namespace RSCoin::Config {

    namespace {

        core::Result<void> decode(const nlohmann::json& value, bool& out) {
            if (!value.is_boolean())
                return core::fail(core::ErrorCode::config, "expected a boolean");
            out = value.get<bool>();
            return {};
        }

        core::Result<void> decode(const nlohmann::json& value, std::uint64_t& out) {
            if (!value.is_number_unsigned())
                return core::fail(core::ErrorCode::config, "expected an unsigned integer");
            out = value.get<std::uint64_t>();
            return {};
        }

        template <typename T>
            requires std::is_unsigned_v<T>
        core::Result<void> decodeNarrow(const nlohmann::json& value, T& out) {
            std::uint64_t raw{};
            if (auto ok = decode(value, raw); !ok)
                return ok;
            if (raw > std::numeric_limits<T>::max())
                return core::fail(core::ErrorCode::config, "value out of range");
            out = static_cast<T>(raw);
            return {};
        }

        core::Result<void> decode(const nlohmann::json& value, std::uint16_t& out) { return decodeNarrow(value, out); }
        core::Result<void> decode(const nlohmann::json& value, std::uint32_t& out) { return decodeNarrow(value, out); }

        core::Result<void> decode(const nlohmann::json& value, std::string& out) {
            if (!value.is_string())
                return core::fail(core::ErrorCode::config, "expected a string");
            out = value.get<std::string>();
            return {};
        }

        core::Result<void> decode(const nlohmann::json& value, core::Address& out) {
            std::string hex;
            if (auto ok = decode(value, hex); !ok)
                return ok;
            auto address = core::fixedFromHex<20>(hex);
            if (!address)
                return core::fail(core::ErrorCode::config, address.error().message);
            out = *address;
            return {};
        }

        core::Result<void> decode(const nlohmann::json& value, core::Bytes& out) {
            std::string hex;
            if (auto ok = decode(value, hex); !ok)
                return ok;
            auto bytes = core::fromHex(hex);
            if (!bytes)
                return core::fail(core::ErrorCode::config, bytes.error().message);
            out = std::move(*bytes);
            return {};
        }

        core::Result<void> decode(const nlohmann::json& value, core::Amount& out) {
            return decode(value, out.units);
        }

        template <typename T>
        core::Result<void> decode(const nlohmann::json& value, std::vector<T>& out) {
            if (!value.is_array())
                return core::fail(core::ErrorCode::config, "expected an array");
            out.clear();
            out.reserve(value.size());
            for (const auto& element : value) {
                T item{};
                if (auto ok = decode(element, item); !ok)
                    return ok;
                out.push_back(std::move(item));
            }
            return {};
        }

        core::Result<void> decode(const nlohmann::json& value, std::map<std::string, std::string>& out) {
            if (!value.is_object())
                return core::fail(core::ErrorCode::config, "expected an object of strings");
            out.clear();
            for (const auto& [key, element] : value.items()) {
                std::string item;
                if (auto ok = decode(element, item); !ok)
                    return core::fail(ok.error(), "entry '" + key + "'");
                out.emplace(key, std::move(item));
            }
            return {};
        }

        core::Result<const nlohmann::json*> find(const nlohmann::json& node, std::string_view name, std::string_view key) {
            const auto it = node.find(key);
            if (it == node.end())
                return core::fail(core::ErrorCode::config, "missing key '" + std::string(key) + "' in section '" + std::string(name) + "'");
            return &*it;
        }

    }

    Section::Section(const nlohmann::json& node, std::string name) : _node(&node), _name(std::move(name)) {}

    template <SectionValue T>
    core::Result<T> Section::get(std::string_view key) const {
        const auto found = find(*_node, _name, key);
        if (!found)
            return core::fail(found.error());

        T out{};
        if (auto ok = decode(**found, out); !ok)
            return core::fail(ok.error(), "key '" + std::string(key) + "' in section '" + _name + "'");
        return out;
    }

    template core::Result<bool> Section::get(std::string_view) const;
    template core::Result<std::uint16_t> Section::get(std::string_view) const;
    template core::Result<std::uint32_t> Section::get(std::string_view) const;
    template core::Result<std::uint64_t> Section::get(std::string_view) const;
    template core::Result<std::string> Section::get(std::string_view) const;
    template core::Result<std::vector<std::string>> Section::get(std::string_view) const;
    template core::Result<std::vector<std::uint16_t>> Section::get(std::string_view) const;
    template core::Result<std::map<std::string, std::string>> Section::get(std::string_view) const;
    template core::Result<core::Address> Section::get(std::string_view) const;
    template core::Result<core::Bytes> Section::get(std::string_view) const;
    template core::Result<core::Amount> Section::get(std::string_view) const;

    core::Result<std::vector<Section>> Section::list(std::string_view key) const {
        const auto found = find(*_node, _name, key);
        if (!found)
            return core::fail(found.error());
        if (!(*found)->is_array())
            return core::fail(core::ErrorCode::config, "key '" + std::string(key) + "' in section '" + _name + "': expected an array");

        std::vector<Section> out;
        out.reserve((*found)->size());
        for (const auto& element : **found) {
            const auto label = _name + "." + std::string(key) + "[" + std::to_string(out.size()) + "]";
            if (!element.is_object())
                return core::fail(core::ErrorCode::config, label + ": expected an object");
            out.push_back(Section(element, label));
        }
        return out;
    }

}

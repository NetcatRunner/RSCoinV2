#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Config {

    namespace detail {
        template <typename T, typename... Options>
        concept OneOf = (std::is_same_v<T, Options> || ...);
    }

    // Types lisibles depuis une section (Address et Bytes attendent de l'hexa).
    template <typename T>
    concept SectionValue = detail::OneOf<T,
        bool, std::uint16_t, std::uint32_t, std::uint64_t,
        std::string, std::vector<std::string>, std::vector<std::uint16_t>,
        std::map<std::string, std::string>,
        core::Address, core::Bytes, core::Amount>;

    // Vue non propriétaire sur une section du fichier de configuration.
    // La façade s'arrête ici : les modules lisent des valeurs typées et ne
    // savent pas que le format sous-jacent est du JSON.
    class Section {
    public:
        template <SectionValue T>
        core::Result<T> get(std::string_view key) const;

        // Tableaux d'objets (ex. genesis.allocations).
        core::Result<std::vector<Section>> list(std::string_view key) const;

        std::string_view name() const noexcept { return _name; }

    private:
        friend class Store;
        Section(const nlohmann::json& node, std::string name);

        const nlohmann::json* _node;
        std::string _name;
    };

    // Lit plusieurs clés en ne retenant que la première erreur :
    //     Config::Reader reader(section);
    //     reader.read("port", config.port);
    //     return reader.finish(std::move(config));
    class Reader {
    public:
        explicit Reader(const Section& section) : _section(section) {}

        template <SectionValue T>
        Reader& read(std::string_view key, T& out) {
            if (_error)
                return *this;
            auto value = _section.get<T>(key);
            if (!value)
                _error = value.error();
            else
                out = std::move(*value);
            return *this;
        }

        template <typename T>
        core::Result<T> finish(T value) const {
            if (_error)
                return core::fail(*_error);
            return value;
        }

    private:
        const Section& _section;
        std::optional<core::Error> _error;
    };

}

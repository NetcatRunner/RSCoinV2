#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Config {

    template <typename T>
    concept ConfigSection = requires(const Section& section) {
        { T::kSection } -> std::convertible_to<std::string_view>;
        { T::from(section) } -> std::same_as<core::Result<T>>;
    };

    class Store {
    public:
        static core::Result<Store> load(const std::filesystem::path& path);

        template <ConfigSection T>
        core::Result<T> get() const {
            const auto found = section(T::kSection);
            if (!found)
                return core::fail(found.error());
            return T::from(*found);
        }

    private:
        Store(std::shared_ptr<const nlohmann::json> root) : _root(std::move(root)) {}

        core::Result<Section> section(std::string_view name) const;

        std::shared_ptr<const nlohmann::json> _root;
    };

}

#include "config/Store.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

namespace RSCoin::Config {

    core::Result<Store> Store::load(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file.is_open())
            return core::fail(core::ErrorCode::config, "cannot open configuration file '" + path.string() + "'");

        try {
            return Store(std::make_shared<const nlohmann::json>(nlohmann::json::parse(file)));
        } catch (const nlohmann::json::parse_error& error) {
            return core::fail(core::ErrorCode::config, "JSON syntax error at byte " + std::to_string(error.byte) + ": " + error.what());
        }
    }

    core::Result<Section> Store::section(std::string_view name) const {
        const auto it = _root->find(name);
        if (it == _root->end())
            return core::fail(core::ErrorCode::config, "missing section '" + std::string(name) + "'");
        if (!it->is_object())
            return core::fail(core::ErrorCode::config, "section '" + std::string(name) + "' must be an object");
        return Section(*it, std::string(name));
    }

}

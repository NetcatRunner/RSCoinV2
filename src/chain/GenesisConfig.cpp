#include "chain/GenesisConfig.hpp"

namespace RSCoin::Chain {

    namespace {
        core::Result<GenesisAccount> readAccount(const Config::Section& entry) {
            GenesisAccount account;
            Config::Reader reader(entry);
            reader.read("address", account.address);
            reader.read("balance", account.balance);
            return reader.finish(std::move(account));
        }
    }

    core::Result<GenesisConfig> GenesisConfig::from(const Config::Section& section) {
        GenesisConfig config;
        Config::Reader reader(section);
        reader.read("timestamp", config.timestamp);
        reader.read("consensusSeal", config.consensusSeal);

        const auto allocations = section.list("allocations");
        if (!allocations)
            return core::fail(allocations.error());
        for (const auto& entry : *allocations) {
            auto account = readAccount(entry);
            if (!account)
                return core::fail(account.error());
            config.allocations.push_back(std::move(*account));
        }
        return reader.finish(std::move(config));
    }

}

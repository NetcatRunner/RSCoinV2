#include "node/Factory.hpp"

#include <utility>

#include "chain/ChainConfig.hpp"
#include "chain/Factory.hpp"
#include "chain/genesis/Genesis.hpp"
#include "consensus/Factory.hpp"
#include "crypto/Factory.hpp"
#include "log/Logger.hpp"
#include "mempool/Factory.hpp"
#include "mining/Factory.hpp"
#include "network/Factory.hpp"
#include "protocol/Factory.hpp"
#include "rpc/Factory.hpp"
#include "state/Factory.hpp"
#include "storage/Factory.hpp"

namespace RSCoin::Node {

    core::Result<Node> makeNode(const Config::Store& network, const Config::Store& node) {
        auto chainConfig = network.get<Chain::ChainConfig>();
        if (!chainConfig)
            return core::fail(chainConfig.error());

        auto crypto = network.get<Crypto::CryptoConfig>().and_then(Crypto::makeProvider);
        if (!crypto)
            return core::fail(crypto.error());

        auto storage = node.get<Storage::StorageConfig>().and_then(Storage::makeStore);
        if (!storage)
            return core::fail(storage.error());

        auto genesisConfig = network.get<Chain::GenesisConfig>();
        if (!genesisConfig)
            return core::fail(genesisConfig.error());

        auto state = network.get<State::StateConfig>().and_then([&](const auto& rules) {
            return State::makeStateMachine(**crypto, chainConfig->id, rules, *genesisConfig);
        });
        if (!state)
            return core::fail(state.error());

        const Primitives::Block genesis = Chain::buildGenesis(*genesisConfig, (*state)->stateRoot());
        auto chain = Chain::makeBlockchain(**storage, (*crypto)->hasher(), genesis);
        if (!chain)
            return core::fail(chain.error());

        auto consensus = network.get<Consensus::ConsensusConfig>().and_then([&](const auto& config) {
            return Consensus::makeEngine(config, **crypto);
        });
        if (!consensus)
            return core::fail(consensus.error());

        auto manager = Chain::makeChainManager(Chain::ChainManagerDeps{**chain, **consensus, (*crypto)->hasher()}, std::move(*state));
        if (!manager)
            return core::fail(manager.error());

        auto mempool = Mempool::makeMempool(**manager);
        if (!mempool)
            return core::fail(mempool.error());

        auto transport = node.get<Network::NetworkConfig>().and_then(Network::makeNetwork);
        if (!transport)
            return core::fail(transport.error());

        auto protocol = network.get<Protocol::ProtocolConfig>().and_then([&](const auto& config) {
            return Protocol::makeProtocol(config, chainConfig->id, **transport, Protocol::NodeServices{**chain, **manager, (*crypto)->hasher(), **mempool});
        });
        if (!protocol)
            return core::fail(protocol.error());

        auto rpcConfig = node.get<Rpc::RpcConfig>();
        if (!rpcConfig)
            return core::fail(rpcConfig.error());

        std::unique_ptr<Rpc::IRpcServer> rpc;
        if (rpcConfig->enabled) {
            auto made = Rpc::makeRpcServer(*rpcConfig, Rpc::NodeServices{**chain, **manager, **mempool, **transport, (*crypto)->hasher()});
            if (!made)
                return core::fail(made.error());
            rpc = std::move(*made);
        }

        auto miningConfig = node.get<Mining::MiningConfig>();
        if (!miningConfig)
            return core::fail(miningConfig.error());

        std::unique_ptr<Mining::IMiner> miner;
        if (miningConfig->enabled) {
            auto made = network.get<Chain::BlockConfig>().and_then([&](auto block) {
                return Mining::makeMiner(Mining::MinerDeps{**chain, **consensus, **manager, **mempool}, std::move(*miningConfig), std::move(block));
            });
            if (!made)
                return core::fail(made.error());
            miner = std::move(*made);
        }

        RSCoin_INFO("starting node (chainId {}, consensus '{}', protocol '{}')", chainConfig->id, (*consensus)->name(), (*protocol)->name());

        return Node{Modules{
            .crypto = std::move(*crypto),
            .storage = std::move(*storage),
            .chain = std::move(*chain),
            .consensus = std::move(*consensus),
            .mempool = std::move(*mempool),
            .manager = std::move(*manager),
            .network = std::move(*transport),
            .protocol = std::move(*protocol),
            .rpc = std::move(rpc),
            .miner = std::move(miner),
        }};
    }

}

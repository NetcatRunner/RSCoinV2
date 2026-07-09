# RSCoin2 — Architecture

## Vision

Un nœud blockchain où chaque sous-système — consensus, réseau, stockage, cryptographie,
machine à états/mempool — est un composant interchangeable derrière une interface pure.
Le nœud lui-même ne connaît aucun type concret : il est assemblé par injection de
dépendances au démarrage, à partir d'un fichier de configuration.

Inspirations : l'interface `consensus.Engine` de go-ethereum (consensus pluggable),
la séparation exécution/consensus d'Ethereum, le header à données de scellement opaques.

## Directives → garanties techniques

| Directive | Mécanisme qui la garantit |
|---|---|
| Modularité extrême | Chaque module = un dossier sous `src/` avec son namespace, qui héberge son propre contrat (`I*.hpp`). Règle d'includes : d'un autre module on n'inclut QUE son contrat — jamais une implémentation. |
| Consensus pluggable | `IConsensus` = `verify` / `prepare` / `seal` / `compare`. Les données propres à un moteur (nonce PoW, signatures PoA, attestations PoS) vivent dans `BlockHeader::consensusSeal` (octets opaques). Le moteur est choisi par `consensus.engine` dans la config, résolu par sa `Factory`. |
| Zéro hardcoding | Genesis (timestamp, allocations, seal initial), chainId, ports, backends, paramètres moteur : tout vient du fichier de configuration (`Config::Store`, sections typées par module). Dans le code : constantes nommées uniquement. |
| C++ moderne | C++23. `std::expected` aux frontières de modules (pas d'exceptions dans les contrats). `std::stop_token` pour l'annulation (seal PoW interruptible). `unique_ptr` aux frontières d'ownership, références partout ailleurs. Agrégats (`Modules`, `WriteBatch`, configs) pour respecter ≤ 3 paramètres par méthode / ≤ 4 par fonction. |

## Arborescence

**Convention** : la racine d'un module contient son **contrat** (`I*.hpp`), sa **config**
(`*Config.hpp`, section typée du fichier JSON) et sa **Factory** (seule surface visible de
`main.cpp`). Chaque **implémentation** vit dans un sous-dossier : nommé d'après la
**variante** que la config sélectionne (`tcp/`, `file/`, `pow/`…), ou d'après son **rôle**
quand l'implémentation est canonique (`chain/manager/`, `mining/local/`). Ajouter une
variante = ajouter un dossier + une entrée dans la Factory, rien d'autre ne bouge.

```
RSCoin2/
├── CMakeLists.txt                  # unique : GLOB_RECURSE src/*.cpp → exécutable RSCoin2
├── ARCHITECTURE.md
├── config/
│   ├── networks/                   # LE RÉSEAU, partagé entre pairs (l'avoir en commun = même chaîne) :
│   │   ├── mainnet.json            #   chain, genesis, block, consensus, crypto, state, protocol
│   │   └── testnet.json
│   └── node.json                   # TOI (gitignoré) : network (p2p), storage, rpc, wallet, mining
└── src/
    ├── main.cpp                    # point d'entrée : CLI + fichiers de config + run — rien d'autre
    ├── core/                       # Types, Result (std::expected), Hex — n'inclut RIEN d'autre
    ├── primitives/                 # Block, Transaction, Codec canonique — n'inclut que core/
    ├── config/                     # Store (get<T>() unique) + Section/Reader (façade sans JSON)
    ├── log/                        # Logger (RST) — infrastructure, comme core/
    ├── utils/                      # SignalHandler…
    ├── crypto/                     # ICrypto + CryptoConfig + Factory
    │   ├── sha256/                 #   hasher "sha256d" (+ algorithme SHA-256)
    │   ├── secp256k1/              #   schéma "secp256k1" (ECDSA, libsecp256k1)
    │   └── stub/                   #   schéma "insecure-stub" (dev uniquement)
    ├── storage/                    # IStorage + StorageConfig + Factory
    │   ├── memory/                 #   backend "memory"
    │   └── file/                   #   backend "file"
    ├── network/                    # TRANSPORT : des octets, aucune sémantique (+ Socket, boîte à outils)
    │   └── tcp/                    #   transport "tcp" (TcpNetwork, Wire, Peer)
    ├── protocol/                   # SÉMANTIQUE : IProtocol + boîte à outils (Message, Dispatcher, Codec)
    │   └── rscoin/                 #   protocole "rscoin" (Status, blocs, transactions, ping/pong)
    ├── consensus/                  # IConsensus + ConsensusConfig + Factory
    │   └── pow/                    #   moteur "pow" (pos/, poa/ à venir)
    ├── state/                      # IStateMachine (immuable) + StateConfig + Factory
    │   └── account/                #   modèle par comptes (à la Ethereum)
    ├── mempool/                    # IMempool + Factory
    │   └── simple/                 #   pool FIFO auto-nettoyant
    ├── mining/                     # IMiner + MiningConfig + Factory
    │   └── local/                  #   mineur local : son propre thread, seal via IConsensus
    ├── chain/                      # IBlockchain/IChainManager/IChainView + configs + Factory
    │   ├── kv/                     #   Blockchain : ARBRE de blocs sur IKeyValueStore (branches conservées)
    │   ├── manager/                #   ChainManager : voie d'écriture unique + fork choice (reorgs)
    │   └── genesis/                #   construction du bloc 0 depuis la config
    ├── rpc/                        # INodeApi (l'API, typée) + IRpcServer + ITransport (coutures) + Api (DTOs)
    │   ├── node/                   #   SÉMANTIQUE : NodeApi sur les services du nœud — zéro JSON
    │   ├── jsonrpc/                #   PROTOCOLE : enveloppe 2.0 + Codec DTOs↔JSON + proxy client
    │   └── http/                   #   TRANSPORT : POST / (curl-able) — porte des octets, zéro JSON
    ├── wallet/                     # IWallet + WalletConfig + Factory — binaire séparé rscoin-wallet
    │   ├── local/                  #   wallet logiciel local (hardware/remote = frères futurs)
    │   ├── keystore/               #   clés sur IKeyValueStore (⚠ en clair, chiffrement à venir)
    │   └── cli/                    #   main.cpp du binaire wallet
    └── node/                       # Node (cycle de vie, contrats uniquement) + Factory = LA composition root
```

## Graphe de dépendances (règle d'or)

La discipline se lit dans les `#include` (chemins relatifs à `src/`) : **la racine
d'un module est son API publique** (contrats `I*.hpp`, configs, Factory, boîte à
outils comme `network/Socket` ou `protocol/Dispatcher`) — **ses sous-dossiers sont
privés**, jamais inclus depuis un autre module (`network/tcp/…`, `consensus/pow/…`).
Les `Factory` restent réservées aux composition roots (`node/Factory`, `wallet/cli/`).

- `core/` → rien ; `primitives/` → `core/` ; `config/` → `core/` ; `log/`, `utils/` → infrastructure
- modules d'implémentation (`crypto/`, `storage/`, `network/`, `protocol/`, `consensus/`,
  `state/`, `mempool/`, `chain/`) → `core/` + `primitives/` + `config/` + `log/` +
  les `I*.hpp` des autres modules
- `node/` → les contrats uniquement — **jamais un type concret**
- `node/Factory` → toutes les factories — **la composition root, seul endroit où les concrets
  sont assemblés** ; `main.cpp` n'est que le point d'entrée (CLI, logs, signaux)

## Réseau et protocole : deux couches, deux contrats

Séparation inspirée de devp2p (Ethereum) : le transport ne sait pas ce qu'est un
bloc ; le protocole ne sait pas ce qu'est une socket.

- **`network/` (transport)** — `INetwork` : start/stop, connect/disconnect,
  send/broadcast de `NetMessage{topic, payload}`. `TcpNetwork` frame les octets
  (`Wire`, borné par `network.maxMessageBytes`), 1 thread lecteur par pair,
  pairs d'amorçage depuis `network.bootstrapPeers`. Les callbacks de l'observer
  peuvent venir de plusieurs threads : l'observer synchronise.
- **`protocol/` (sémantique)** — `IProtocol` (est l'`INetworkObserver` du nœud) :
  interprète les topics et répond via `INetwork`. Le protocole est choisi par
  `protocol.name` en config, exactement comme le moteur de consensus. `rscoin/1`
  fait la poignée de main d'Ethereum (Status mutuel : version + chainId, sinon
  déconnexion) et le keepalive ping/pong piloté par `tick()` (appelé par `Node`).
  Implémenter un protocole Bitcoin ou Ethereum = un nouveau dossier sous
  `protocol/` + une entrée dans sa Factory. Rien d'autre ne bouge.

## Les contrats

- **`IConsensus`** — `verify(header, chain)` valide un header selon les règles du moteur ;
  `prepare(draft, chain)` remplit les champs moteur d'un bloc en production ;
  `seal(draft, chain, cancel)` scelle (minage PoW long et annulable, signature PoA
  instantanée) ; `compare(lhs, rhs, chain)` est la règle de choix de fourche.
  Le moteur lit l'historique via `IChainView` : jamais le stockage directement.
- **`ICryptoProvider`** — agrège `IHasher` et `ISignatureScheme` (interfaces séparées :
  ISP ; un moteur qui ne signe pas ne dépend pas de la signature). Tailles de clés et
  signatures variables (`Bytes`) : le schéma est interchangeable (secp256k1, ed25519…).
- **`IKeyValueStore`** — get/put/erase + `apply(WriteBatch)` atomique. Les dépôts de
  domaine (blocs, état) se construiront au-dessus, dans `chain/` et `state/`.
- **`INetwork`** — transport d'octets par topics, pilotée par `INetworkObserver`.
  Le réseau ne connaît ni bloc ni transaction : la couche protocole (dans `node`)
  possède l'espace des topics.
- **`IStateMachine`** — couche exécution : valide et applique les transactions,
  expose la racine d'état. Totalement ignorante du consensus (séparation
  exécution/consensus, comme Ethereum).

## Réseau vs nœud : deux fichiers, deux rôles

Le modèle geth/substrate : un fichier **réseau** (`config/networks/*.json`) contient
tout ce qui définit le consensus — le partager à l'identique, c'est être sur la même
chaîne (créer un réseau privé = écrire un fichier et l'envoyer à ses pairs). Le
fichier **personnel** (`config/node.json`, gitignoré) contient ce qui n'appartient
qu'à ce nœud : ports, pairs d'amorçage, stockage, RPC, keystore, minage. La
composition root charge les deux Stores et lit chaque section dans le bon — aucun
module ne connaît cette séparation.

```
./RSCoin2       --network config/networks/mainnet.json  -c config/node.json   (= défauts)
./rscoin-wallet --network config/networks/testnet.json  -c config/node.json
```

## Le test du consensus

1. `./RSCoin2 -n config/networks/mainnet.json` → nœud PoW.
2. `./RSCoin2 -n config/networks/testnet.json` → autre moteur. **Aucune
   recompilation, zéro ligne de code modifiée** : seul le fichier réseau diffère.
3. Ajouter un moteur = créer `src/consensus/<nom>/` + une entrée dans
   `consensus/Factory`. Rien d'autre ne bouge.

## Modèle de threading

Les modules sont passifs ; l'orchestration appartient à `Node`. Les opérations longues
prennent un `std::stop_token` (seal PoW). Le réseau notifie via `INetworkObserver` ;
c'est `Node` qui sérialise les accès aux autres modules.

## Blocs évolutifs et configurables

Le bloc (dans `src/primitives/`) évolue **par ajout, jamais par modification** — le
mécanisme d'Ethereum :

- `BlockHeader::extraData` : octets libres, bornés par `block.maxExtraDataBytes` en config.
- `BlockHeader::extensions` : liste de `HeaderExtension{tag, payload}` opaques. Un fork qui
  ajoute `receiptsRoot`, `gasLimit/gasUsed` ou `baseFee` = une extension de plus, activée
  par `block.enabledExtensions` dans le fichier de configuration — le cœur du header ne
  change jamais. Accès par `findExtension` / `setExtension`.
- `Transaction::type` (façon EIP-2718) : les variantes futures (frais dynamiques, blobs…)
  se discriminent par le type sans casser le format existant.
- `BlockHeader::version` : garde-fou si une rupture de structure devenait inévitable.

Des blocs « à la Ethereum » s'obtiennent donc en éditant la section `block` de la config,
pas en modifiant `primitives/`. La validation de forme (`BlockConfig`) sera appliquée par
les modules `chain`/`state` à venir.

## Décisions structurantes (à valider ensemble)

1. **Seal opaque** dans `BlockHeader::consensusSeal` : le core ignore tout de PoW/PoS/PoA.
2. **Modèle d'état par comptes** (Ethereum-like : `from/to/value/nonce`), pas UTXO.
3. **`std::expected` partout** aux frontières ; exceptions bannies des contrats.
4. **Paramètres moteur en `map<string, string>`** — simple pour démarrer ; évoluera vers
   des sections typées par moteur si besoin.
5. **Tests contractuels** : une suite par interface, que chaque implémentation doit
   passer (ex. la même suite valide `MemoryKeyValueStore` et `FileKeyValueStore`).

## Transactions signées : le circuit complet

Les clés n'entrent jamais dans le nœud. Le binaire `rscoin-wallet` (même fichier de
configuration que le nœud → crypto/chainId/RPC cohérents par construction) signe
localement et soumet par JSON-RPC :

- digest signé = `hasher(encodeForSigning(tx, chainId))` (la tx sans son champ
  signature + chainId, anti-rejeu façon EIP-155 — dans `primitives/Codec`) ;
- la `Signature` est un blob **autoportant** dont le format appartient au schéma
  (`[pubkey][sig]`) ; `ISignatureScheme::authenticate(digest, blob) → Address`
  rend le schéma totalement interchangeable (le validateur ne connaît ni pubkey
  ni format) ;
- `AccountStateMachine` vérifie `authenticate(...) == tx.from` pour TOUTE
  transaction (mempool et blocs) — une tx non/mal signée est `rejected`/`invalid`.

`./rscoin-wallet --new | --list | --balance 0x… | --send --to 0x… --value N`

## Reorgs : le choix de fourche appliqué

Le dépôt de blocs est un **arbre** (à la geth) : un bloc qui ne prolonge pas la tête
est conservé (`storeBlock`) au lieu d'être jeté. `ChainManager` reste l'unique voie
d'écriture : si `consensus.compare` juge la branche latérale meilleure, `reorgLocked`
remonte la branche au tronc commun, la **rejoue depuis le genesis** (stateRoots
vérifiés — une branche invalide annule tout, l'ancienne chaîne reste), puis
`adoptBranch` bascule l'index canonique en un batch atomique. Le mempool n'a rien à
savoir : son éviction paresseuse élimine les transactions invalidées par la bascule.
Côté sync, si un batch reçu ne se raccorde pas, le protocole redemande une fenêtre
plus tôt jusqu'au tronc commun (et déconnecte si les genesis diffèrent).

## État actuel et prochaines étapes

Le nœud est fonctionnel de bout en bout : boot 100 % config (genesis compris), PoW
sur thread dédié, persistance/redémarrage, sync P2P avec reorgs (deux mineurs
concurrents convergent), relais de transactions, wallet CLI + RPC, signatures ECDSA
réelles (`secp256k1`) vérifiées par le consensus.

1. retargeting de la difficulté PoW ; snapshot d'état persistant au démarrage
   (les reorgs profonds rejouent depuis le genesis — un cache d'états l'évitera) ;
2. chiffrement du keystore (scrypt+AES) ; frais de transaction + tri du mempool ;
3. moteurs `pos`/`poa`.

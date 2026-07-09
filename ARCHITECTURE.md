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
`main.cpp`). Chaque **implémentation** vit dans un sous-dossier portant le nom de la
variante que la config sélectionne — ajouter une variante = ajouter un dossier + une
entrée dans la Factory, rien d'autre ne bouge.

```
RSCoin2/
├── CMakeLists.txt                  # unique : GLOB_RECURSE src/*.cpp → exécutable RSCoin2
├── ARCHITECTURE.md
├── config/
│   └── node.pow.json               # tout le nœud vient d'ici (genesis compris)
└── src/
    ├── main.cpp                    # composition root : seul endroit où les concrets sont assemblés
    ├── core/                       # Types, Result (std::expected), Hex — n'inclut RIEN d'autre
    ├── primitives/                 # Block, Transaction, Codec canonique — n'inclut que core/
    ├── config/                     # Store (get<T>() unique) + Section/Reader (façade sans JSON)
    ├── log/                        # Logger (RST) — infrastructure, comme core/
    ├── utils/                      # SignalHandler…
    ├── crypto/                     # ICrypto + CryptoConfig + Factory
    │   ├── sha256/                 #   hasher "sha256d" (+ algorithme SHA-256)
    │   └── stub/                   #   schéma "insecure-stub" (secp256k1/ à venir)
    ├── storage/                    # IStorage + StorageConfig + Factory
    │   ├── memory/                 #   backend "memory"
    │   └── file/                   #   backend "file"
    ├── network/                    # TRANSPORT : des octets, aucune sémantique
    │   └── tcp/                    #   transport "tcp" (TcpNetwork, Socket, Wire, Peer)
    ├── protocol/                   # SÉMANTIQUE : IProtocol + boîte à outils (Message, Dispatcher, Codec)
    │   └── rscoin/                 #   protocole "rscoin" (Status, blocs, transactions, ping/pong)
    ├── consensus/                  # IConsensus + ConsensusConfig + Factory
    │   └── pow/                    #   moteur "pow" (pos/, poa/ à venir)
    ├── state/                      # IStateMachine (immuable) + StateConfig + Factory
    │   └── account/                #   modèle par comptes (à la Ethereum)
    ├── mempool/                    # IMempool + Factory
    │   └── simple/                 #   pool FIFO auto-nettoyant
    ├── mining/                     # IMiner + MiningConfig + Factory + Miner (générique : pas de variante)
    ├── chain/                      # IBlockchain/IChainManager + Blockchain/ChainManager/Genesis (canoniques)
    └── node/                       # le nœud — ne voit QUE les contrats
```

## Graphe de dépendances (règle d'or)

La discipline se lit dans les `#include` (chemins relatifs à `src/`) : d'un autre
module, on n'inclut **que son contrat `I*.hpp`** — jamais une implémentation
(`TcpNetwork.hpp`, `RSCoinProtocol.hpp`…), ni une `Factory` (réservée à `main.cpp`).

- `core/` → rien ; `primitives/` → `core/` ; `config/` → `core/` ; `log/`, `utils/` → infrastructure
- modules d'implémentation (`crypto/`, `storage/`, `network/`, `protocol/`, `consensus/`,
  `state/`, `mempool/`, `chain/`) → `core/` + `primitives/` + `config/` + `log/` +
  les `I*.hpp` des autres modules
- `node/` → les contrats uniquement — **jamais un type concret**
- `main.cpp` → `node/` + les factories — **seul endroit où les concrets existent**

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

## Le test du consensus

1. `./RSCoin2 config/node.pow.json` → nœud PoW.
2. `./RSCoin2 config/node.poa.json` → nœud PoA. **Aucune recompilation, zéro ligne
   de code modifiée** : seule la valeur `consensus.engine` diffère.
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

## État actuel et prochaines étapes

Le nœud est fonctionnel de bout en bout : il démarre depuis le JSON (genesis compris),
mine en PoW sur son propre thread, persiste et redémarre à la bonne hauteur, se
synchronise entre pairs (rattrapage + suivi en direct) et relaie les transactions
(admission validée par le mempool, anti-écho par pair).

1. soumission de transactions (wallet : clés + signature, puis CLI/RPC) ;
2. vraie crypto : `crypto/secp256k1/` + vérification des signatures dans `state/` ;
3. reorgs (`consensus.compare` + stockage des chaînes latérales) ;
4. retargeting de la difficulté PoW ; snapshot d'état persistant au démarrage.

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
| Modularité extrême | Chaque module = un dossier sous `src/` avec son namespace. Communication uniquement via `src/interfaces/` (classes pures). Règle d'includes : un module d'implémentation n'inclut JAMAIS un autre module d'implémentation — uniquement `core/`, `interfaces/`, `config/`. |
| Consensus pluggable | `IConsensus` = `verify` / `prepare` / `seal` / `compare`. Les données propres à un moteur (nonce PoW, signatures PoA, attestations PoS) vivent dans `BlockHeader::consensusSeal` (octets opaques). Le moteur est choisi par `consensus.engine` dans la config, résolu par `EngineFactory`. |
| Zéro hardcoding | Genesis (timestamp, allocations, seal initial), chainId, ports, backends, paramètres moteur : tout vient de `NodeConfig`, chargé depuis un fichier au démarrage. Dans le code : constantes nommées uniquement. |
| C++ moderne | C++23. `std::expected` aux frontières de modules (pas d'exceptions dans les contrats). `std::stop_token` pour l'annulation (seal PoW interruptible). `unique_ptr` aux frontières d'ownership, références partout ailleurs. Agrégats (`Modules`, `WriteBatch`, configs) pour respecter ≤ 3 paramètres par méthode / ≤ 4 par fonction. |

## Arborescence

```
RSCoin2/
├── CMakeLists.txt                  # unique : GLOB_RECURSE src/*.cpp → exécutable RSCoin2
├── ARCHITECTURE.md
├── config/
│   ├── node.pow.json               # même nœud, moteur PoW
│   └── node.poa.json               # même nœud, moteur PoA — seule la config change
└── src/
    ├── main.cpp                    # composition root : seul endroit où les concrets sont assemblés
    ├── core/                       # types de base — n'inclut RIEN d'autre
    │   ├── Types.hpp               # Bytes, Hash256, Address, Amount, clés…
    │   └── Result.hpp              # Error + Result<T> (std::expected)
    ├── primitives/                 # objets du domaine — n'inclut que core/
    │   ├── Transaction.hpp         # transaction typée (façon EIP-2718)
    │   └── Block.hpp               # BlockHeader (seal opaque + extraData + extensions), Block
    ├── config/                     # NodeConfig + chargement depuis un fichier
    ├── log/                        # Logger (RST) — infrastructure, comme core/
    ├── utils/                      # SignalHandler…
    ├── crypto/                     # ICrypto.hpp (contrat) + suites concrètes
    ├── storage/                    # IStorage.hpp + backends (memory, file…)
    ├── network/                    # TRANSPORT : des octets, aucune sémantique
    │   ├── INetwork.hpp            # contrat + Endpoint/PeerId/NetMessage/INetworkObserver
    │   ├── Socket.hpp/.cpp         # socket TCP POSIX en RAII (détail privé)
    │   ├── Wire.hpp                # framing [topic:2][length:4][payload] (détail privé)
    │   ├── TcpNetwork.hpp/.cpp     # accept-thread + 1 reader-thread par pair
    │   └── Factory.hpp/.cpp        # route config network.transport ("tcp")
    ├── protocol/                   # SÉMANTIQUE : pluggable, comme le consensus
    │   ├── IProtocol.hpp           # INetworkObserver + name() + tick()
    │   ├── Factory.hpp/.cpp        # route config protocol.name ("rscoin", demain "bitcoin"…)
    │   └── rscoin/                 # 1er protocole : Status{version,chainId} + ping/pong
    ├── consensus/                  # IConsensus.hpp + EngineFactory + pow/ pos/ poa/ (à venir)
    ├── state/                      # IStateMachine.hpp + implémentations
    ├── mempool/                    # IMempool.hpp + implémentations
    ├── chain/                      # IChainView.hpp (+ GenesisBuilder à venir)
    └── node/                       # le nœud — ne voit QUE les contrats
        └── Node.hpp/.cpp
```

Chaque module héberge son propre contrat (`I*.hpp`) à côté de ses implémentations.

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
3. Ajouter un moteur = créer `src/consensus/<nom>/` (qui n'inclut que
   `core/`+`interfaces/`+`config/`) + une entrée dans `EngineFactory`. Rien d'autre ne bouge.

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

Le squelette **configure, compile, linke et s'exécute** : chaque factory renvoie pour
l'instant une erreur propre (`notImplemented`) — aucune logique métier n'existe encore,
par design.

1. `config::loadFromFile` (parseur JSON, dépendance vendorisée) ;
2. implémentations de référence : `storage/memory`, `network/loopback` + premiers tests
   contractuels ;
3. module `chain` : `IChainView` sur `IKeyValueStore` + `GenesisBuilder` (genesis 100 % config) ;
4. premier moteur : `consensus/pow`.

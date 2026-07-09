- Creer des tests
- netoyer un peut les fonction dans namespace anonyme
- Fix fast miner sigint

- Vérification des signatures dans l'exécution — la state machine vérifie nonce/solde mais pas encore que la signature de la tx est valide (dépend du #2).

- Reorgs — deux mineurs concurrents finiront par forker ; aujourd'hui le fork est ignoré (forked). Il faut stocker les chaînes latérales + utiliser consensus.compare() pour basculer.
- Retargeting PoW — difficulté fixe = temps de bloc incontrôlé.

- Update AccountStateMachine ppur limité usage de la ram
- Mempool limité usage de la ram aussi -> dans un fichier

- Creer des tests
- netoyer un peut les fonction dans namespace anonyme
- Faire des sous dossier pour rendre l'arborescence et structure un peu plus propre (pour chain/ mining/)

- Un moyen de créer de Tx (wallet : génération de clés, signature, envoi — ou une RPC).
- Vraie crypto secp256k1 — le schéma actuel est insecure-stub (la clé publique révèle la privée). Tant que c'est là, n'importe qui peut voler n'importe qui.
- Vérification des signatures dans l'exécution — la state machine vérifie nonce/solde mais pas encore que la signature de la tx est valide (dépend du #2).

- Reorgs — deux mineurs concurrents finiront par forker ; aujourd'hui le fork est ignoré (forked). Il faut stocker les chaînes latérales + utiliser consensus.compare() pour basculer.
- Retargeting PoW — difficulté fixe = temps de bloc incontrôlé.

- Update AccountStateMachine ppur limité usage de la ram
- Mempool limité usage de la ram aussi -> dans un fichier

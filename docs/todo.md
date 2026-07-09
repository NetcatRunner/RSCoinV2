- Creer des tests
- netoyer un peut les fonction dans namespace anonyme
- Fix fast miner sigint

- Séparer configuration de la blockchain (pour créer des reseau personnalisé = si tout le monde a la meme configuration alors il sont sur leurs reseau custom) et configuration perso (avec adresse, connection sur un pairs)

- Reorgs — deux mineurs concurrents finiront par forker ; aujourd'hui le fork est ignoré (forked). Il faut stocker les chaînes latérales + utiliser consensus.compare() pour basculer.
- Retargeting PoW — difficulté fixe = temps de bloc incontrôlé.

- Update AccountStateMachine ppur limité usage de la ram
- Mempool limité usage de la ram aussi -> dans un fichier
- Clear les thread lorsqu'un node se deconnect

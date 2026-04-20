# Jeu d'Échecs en Réseau

## À propos du projet
Ce projet est un véritable jeu d'échecs multijoueur en réseau (architecture Client/Serveur) développé en C. Il permet à deux joueurs de se connecter à un serveur central pour chercher une partie et s'affronter en 1 contre 1 sur une belle interface graphique (propulsée par Raylib). 

## Comment lancer le jeu ?

Pour démarrer et tester le jeu, veuillez d'abord compiler les sources, puis lancer le serveur et les clients dans des terminaux séparés.

### 1. Recompiler le projet
Assurez-vous de partir sur une version propre grâce aux commandes suivantes :
```bash
make clean
make serveur
make client
```

### 2. Lancer le serveur
Démarrez en premier le serveur en lui fournissant un port de communication au choix (exemple : 8080) :
```bash
./dist/serveur 8080
```

### 3. Lancer vos clients
Une fois le serveur en ligne, vous pouvez démarrer autant de clients que vous le souhaitez ! Lancez la commande suivante (l'IP `127.0.0.1` correspond à votre machine locale) :
```bash
./dist/client 127.0.0.1 8080
```

*Une fois connecté, suivez les instructions dans la console pour créer un compte, chercher une partie et commencer à jouer !*

## Auteurs du projet
Ce projet a été imaginé et développé en équipe par :
- **Figlie Luca**
- **Blanc Valentin**
- **Ait lamine Sohaib**


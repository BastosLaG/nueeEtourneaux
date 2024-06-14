# La Nuée d'Etourneaux en OpenGL avec la librairie GL4D

## Aperçu

Ce projet démontre le rendu d'objets mobiles dans une scène 3D en utilisant OpenGL. Il inclut des fonctionnalités telles que la sélection d'objets, la cartographie des ombres et l'orientation des objets en fonction de leur direction de mouvement. Le programme utilise la bibliothèque GL4D pour les fonctionnalités OpenGL.

## Fonctionnalités

- **Rendu d'Objets** : Rendu de plusieurs objets mobiles dans une scène 3D.
- **Sélection d'Objets** : Sélectionnez des objets à l'aide de la souris et manipulez leurs positions.
- **Cartographie des Ombres** : Implémentez des ombres pour un aspect 3D réaliste.
- **Orientation des Objets** : Orientez les objets en fonction de leur direction de mouvement pour qu'ils fassent face à la direction du déplacement.

## Pré-requis

- OpenGL
- Bibliothèque GL4D
- Bibliothèque Assimp pour le chargement de modèles

## Installation

1. **Cloner le dépôt** :
    ```sh
    git clone git@github.com:BastosLaG/nueeEtourneaux.git
    cd nueeEtourneaux/projet
    ```

2. **Installer les dépendances** :
    Assurez-vous d'avoir OpenGL et la bibliothèque GL4D installés sur votre système. Vous devrez peut-être installer Assimp pour le chargement des modèles.
    ```sh
    sudo apt update
    sudo apt install gcc make libsdl2-dev libsdl2-mixer-dev libsdl2-image-dev libsdl2-ttf-dev libassimp-dev libgl1-mesa-dev libglu1-mesa-dev doxygen upx
    ```
3. **Construire le projet** :
    ```sh
    make
    ```
4. **Exécuter le programme** :
    ```sh
    ./demo
    ```

## Utilisation

### Contrôles de la souris

- **Clic gauche** : Sélectionnez et faites glisser les objets pour les déplacer dans la scène.

### Contrôles du clavier

- **P** : Permet l'apparition et la suppression de notre prédateur
- **m** : Ce met a la place du prédateur
- **v** : Place la vue sur le centre de notre masse d'étourneaux
- **c** : Change la couleur de nos étourneaux 
- **b** : Passe du système "Masse ressort" au système Boids 

## Structure des fichiers

- `sources/` - Fichiers source
- `headers/` - Fichiers d'en-tête
- `shaders/` - Shaders vertex et fragment
- `models/` - Modèles 3D utilisés dans la scène
- `Makefile` - Script de construction

## Contribuer

Les contributions sont les bienvenues !

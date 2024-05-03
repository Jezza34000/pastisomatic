# PastisOmatic

Le distributeur de pastis personnalisé !

## Description du Projet
PastisOmatic est un projet de distributeur de pastis qui permet de mélanger l'eau et le pastis selon l'intensité souhaitée par l'utilisateur. 
Il offre la possibilité de choisir la force de la boisson en ajustant le dosage d'eau et d'alcool via un potentiomètre. 
Le projet utilise un distributeur à eau chaude variable comme base, dont l'intérieur a été vidé et remplacé par un microcontrôleur ESP32 ainsi que deux pompes pour l'eau et l'alcool respectivement.

## Matériel

### Matériel Electronique Utilisé

- ESP32 : Microcontrôleur responsable de la gestion de toutes les opérations, programmé en langage C++.
- IHM (interface homme machine) :
    - Boutons & Leds : Composée de 4 boutons et 4 LEDs intégrées aux boutons
    - Poentiomètre de régale : pour choisir l'intensité du pastis, avec retroéclairage LEDs
    - Écran OLED Monochrome Blanc 1,3" : Affiche les différents messages et commentaires sur la boisson.
- 2 Pompes : Débit d'environ 1 litre par minute chacune, une pour l'eau et une pour l'alcool.
- 1 Relais : Utilisé pour actionner l'électrovanne du réservoir.
- 1 Alimentation 24V 2A : Fournit l'énergie nécessaire au fonctionnement du système.


## Schéma 



## Fonctionnement

### Fonctionnalités Principales
Le distributeur PastisOmatic se compose de 4 boutons avec les fonctionnalités suivantes :

1. **Nettoyer & Preparer** : Active les pompes pendant un temps prédéfini pour amorcer et/ou nettoyer le système.
2. **Ajouter Pastis** : Active la pompe à alcool uniquement pour renforcer le cocktail.
3. **Ajouter Eau** : Active la pompe à eau pour diluer la boisson.
4. **Mix** : Prépare et sert un pastis selon le dosage et la force prédéfinis par le réglage du potentiomètre.

## Code

Le code du projet est principalement écrit en langage C++ et utilise les fonctionnalités de l'ESP32 pour gérer les différentes tâches de contrôle et d'interaction avec le matériel électronique.

### IHM

Les 4 boutons du système sont associés à des interruptions (attachInterrupt) pour déclencher les actions correspondantes.
Le potentiomètre est lu dans la boucle principale (loop) pour mettre à jour une barre de progression sur l'écran.
L'écran affiche différents messages et commentaires sur la boisson en fonction des actions en cours.

### Contrôle des Pompes
Les pompes sont contrôlées par modulation de largeur d'impulsion (PWM), proportionnelle à la valeur du potentiomètre.
Une exception est faite pour la pompe à alcool, qui applique également un PWM "logiciel" avec un cycle de 1 Hz pour réduire la quantité distribuée.

### Bluetooth

Le logiciel est equipé du bluetooth afin de régler les paramètres PWM des pompes


## Simulation du projet

Le projet sur sa conception electronique et test du code à été faite avec Wokwi 

### Wokwi simulaion project address 

https://wokwi.com/projects/388893363315433473

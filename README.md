# HospitalSim — simulateur hospitalier (Qt / C++ / MySQL)

Application Qt 6 Widgets et Qt SQL connectée à MySQL 8+. Les tables et vues sont créées au démarrage si elles n'existent pas. Aucune donnée n'est préchargée.

## Prérequis et configuration

- Serveur MySQL 8+ démarré.
- Qt 6 avec **Widgets**, **Sql** et le pilote SQL Qt **QMYSQL** correspondant à ta version/architecture de Qt.
- CMake 3.21+ et compilateur C++17.

Crée la base et son schéma en exécutant `sql/schema.sql` avec MySQL Workbench ou le client MySQL. Dans CLion, ajoute ces variables dans **Run | Edit Configurations | Environment variables** et remplace le mot de passe par celui de ton serveur :

```text
HOSPITAL_DB_HOST=127.0.0.1
HOSPITAL_DB_PORT=3306
HOSPITAL_DB_NAME=hospital_sim
HOSPITAL_DB_USER=root
HOSPITAL_DB_PASSWORD=ton_mot_de_passe
```

Le mot de passe n'est pas stocké dans le code. Si l'application indique que `QMYSQL` n'est pas disponible, installe le plugin MySQL de Qt pour la même version et architecture que ton installation Qt.

```sh
cmake -S . -B build
cmake --build build
./build/HospitalSim
```

## Utilisation

- **Ajouter patient + parcours** : admission aux urgences et création du parcours initial. Il faut avoir créé un service nommé `Urgences`.
- **Avancer la simulation** : fait avancer les parcours actifs : admission, diagnostic, traitement, sortie.
- Le statut d'un parcours est choisi parmi **En cours**, **Terminé** et **Annulé**.
- L'état de santé est choisi parmi **Stable**, **Aggravation**, **Critique** et **Amélioration**.
- Le libellé du personnel est calculé à partir du nom, prénom et spécialité; il apparaît dans le menu des affectations sans être une colonne séparée.
- La date de dernière maintenance se saisit avec le calendrier et s'enregistre au format `YYYY-MM-DD`.
- Les observations des étapes et évolutions sont demandées à l'ajout. Une valeur vide devient **Aucune observation saisie**.
- Chaque onglet permet d'ajouter, modifier, enregistrer et supprimer les patients, parcours, étapes, évolutions, services, personnel, affectations, lits et équipements.
- Les clés étrangères sont choisies dans des menus lisibles; MySQL stocke l'ID correspondant.

## Fichiers

- `CMakeLists.txt` : configure la compilation et lie Qt Widgets/Sql.
- `src/main.cpp` : initialise MySQL puis ouvre la fenêtre.
- `src/database.h` / `src/database.cpp` : connexion QMYSQL via variables d'environnement, création des tables et vues de libellés.
- `src/mainwindow.h` / `src/mainwindow.cpp` : interface CRUD, menus déroulants, calendrier de maintenance et indicateurs.
- `src/simulationservice.h` / `src/simulationservice.cpp` : opérations métier, transactions d'admission et progression de simulation.
- `sql/schema.sql` : script MySQL complet pour créer la base, tables, clés étrangères et vues.

## Modèle relationnel

`patient` possède des `parcours` et des `evolution_sante`; un parcours contient des `etape_parcours` qui référencent un `service`; les services regroupent lits, personnel et équipements; `affectation_personnel` relie le personnel aux services.

## Limites

Ce MVP fournit un parcours déterministe. Les scénarios d'afflux massif, pannes, capacité réelle des lits, files d'attente, disponibilité du personnel et calculs détaillés de performance restent à ajouter. Ce prototype n'est pas un outil de décision médicale.

# Tracker Modulaire – Entropia Universe

## 📌 Présentation

**Tracker Modulaire** est un programme en **C pur (C99)** destiné aux joueurs de **Entropia Universe**.
Il analyse en temps réel (**LIVE**) ou en relecture (**REPLAY**) le fichier `chat.log` du jeu afin de :

- détecter automatiquement les actions de chasse (shots, loots, kills)
- enregistrer ces événements dans un **CSV structuré**
- calculer des **statistiques précises de session** (loot, dépenses, net, ROI)
- prendre en compte des **armes configurables** via `armes.ini`
- offrir une **interface console interactive**, portable **Linux & Windows**

Le projet est conçu de manière **totalement modulaire**, robuste face aux variations du `chat.log`, et facilement extensible (mining, sweat, autres trackers).

---

## 🎯 Objectifs du projet

- Séparer clairement **parsing / logique / calcul / affichage**
- Éviter les globals incontrôlés
- Être **portable Linux / Windows**
- Respecter une philosophie proche de **l’École 42**
- Faciliter l’ajout de nouvelles règles ou trackers

---

## 🧠 Principe de fonctionnement global

```
chat.log (Entropia)
        ↓
[ parser_engine ]
        ↓
[ hunt_rules ]  → SHOT / LOOT / KILL
        ↓
hunt_log.csv (structuré)
        ↓
[ tracker_stats ] → calculs purs
        ↓
[ tracker_view ] → affichage console
```

---

## 📂 Arborescence du projet

```
tracker_modulaire/
├── bin/                    # Exécutables Linux & Windows
├── build/
│   └── src/                # Fichiers objets (.o)
├── include/                # Headers (.h)
├── logs/                   # Fichiers runtime
│   ├── hunt_log.csv
│   ├── hunt_session.offset
│   └── weapon_selected.txt
├── src/                    # Sources (.c)
├── tests/
│   └── hunt_rules_cases.txt
├── armes.ini                # Configuration des armes
├── Makefile
├── README.md
└── LICENSE
```

---

## 🔫 Configuration des armes – `armes.ini`

Les armes sont **100 % configurables sans recompilation**.

Chaque arme définit :
- DPP
- coût munition / tir
- decay / tir
- decay ampli (optionnel)
- markup

Cela permet :
- d’ajouter/modifier des armes facilement
- de séparer données et logique
- d’avoir des calculs fiables et cohérents

---

## 📦 Description détaillée des modules

### 🔹 main.c
Point d’entrée du programme.
Initialise l’environnement et lance le menu principal.

---

### 🔹 Menus & UI

**menu_principale.c**
- Menu principal intelligent
- Affiche :
  - état du parser (RUNNING / STOPPED)
  - arme active
  - session offset
  - warnings (armes.ini manquant, arme invalide, etc.)
- Permet :
  - démarrer / arrêter le parser
  - afficher les stats
  - reset session / CSV

**menu_tracker_chasse.c**
- Menu dédié à la chasse
- Reload `armes.ini`
- Choix de l’arme active
- Dashboard live auto-refresh

**ui_utils.c**
- Fonctions utilitaires console
- Clear screen
- Sleep portable
- Gestion clavier Linux / Windows

---

### 🔹 Parsing & événements

**parser_engine.c**
- Cœur du parsing
- Mode LIVE (tail du chat.log)
- Mode REPLAY
- Gestion EOF, rotation, troncature
- Appelle `hunt_rules`
- Écrit dans le CSV

**parser_thread.c**
- Parsing dans un thread dédié
- pthread (Linux) / CreateThread (Windows)

**chatlog_path.c**
- Détection automatique du chemin du `chat.log`
- Linux (Wine / Lutris)
- Windows natif

---

### 🔹 Règles de chasse – cœur logique

**hunt_rules.c**
- Analyse une ligne du chat.log
- Produit un événement structuré

Gère :
- SHOT (EN / FR)
- LOOT (quantité, valeur FR/EN)
- KILL (déduplication 1/sec)
- événements pending (loot + kill)

Ignore :
- Rookie / Débutant
- faux positifs système

👉 **Fichier clé si tu veux modifier le comportement du parsing**.

---

### 🔹 CSV & fichiers

**csv.c**
- Écriture CSV sécurisée
- Header automatique

**fs_utils.c**
- Création de dossiers
- Utilitaires filesystem

**core_paths.c**
- Centralisation de tous les chemins runtime

---

### 🔹 Session

**session.c**
- Gestion du fichier `hunt_session.offset`
- Reset session
- Reprise propre

---

### 🔹 Armes

**config_arme.c**
- Chargement de `armes.ini`
- Calcul coût / shot

**weapon_selected.c**
- Sauvegarde de l’arme active

---

### 🔹 Stats & affichage

**tracker_stats.c**
- Calculs purs (sans printf)
- Shots, kills, loot, dépenses, net, ROI
- Testable indépendamment

**tracker_view.c**
- Affichage console formaté
- Dashboard live

---

## 🧪 Tests

```
make test
```

Tests des règles de chasse via `hunt_rules_cases.txt`.
Objectif : **ne jamais casser le parsing** en ajoutant des règles.

---

## ⚠️ Limitations connues

- Le parsing dépend strictement du format du `chat.log` d’Entropia Universe : toute modification majeure côté jeu peut nécessiter une mise à jour des règles.
- Les événements complexes (loot différé, messages retardés) sont gérés au mieux mais restent dépendants de la cohérence du log.
- Le programme est volontairement **console-only** (pas de GUI).
- Une seule session active à la fois (pas de multi-personnage simultané).

---

## 🧠 Diagramme de fonctionnement détaillé

```
┌──────────────┐
│  chat.log    │
│ (EU client) │
└──────┬───────┘
       │ lecture incrémentale
       ▼
┌──────────────┐
│ parser_engine│
│  (LIVE/REPLAY)│
└──────┬───────┘
       │ lignes brutes
       ▼
┌──────────────┐
│ hunt_rules   │◄─── règles EN/FR
│ (SHOT/LOOT/ │
│   KILL)     │
└──────┬───────┘
       │ événements normalisés
       ▼
┌──────────────┐
│ hunt_log.csv │
│  (persistant)│
└──────┬───────┘
       │ données structurées
       ▼
┌──────────────┐
│ tracker_stats│
│ (calcul pur) │
└──────┬───────┘
       │ stats finales
       ▼
┌──────────────┐
│ tracker_view │
│ (console UI) │
└──────────────┘
```

---

## 📄 Format du CSV (`logs/hunt_log.csv`)

Le fichier CSV est créé automatiquement au premier lancement.

### Header
```
timestamp,event_type,value_1,value_2,text
```

### Types d’événements

- `SHOT`
  - value_1 : coût du tir (PED)
- `LOOT`
  - value_1 : valeur loot (PED)
  - value_2 : quantité (si applicable)
- `KILL`
  - value_1 : 1

### Exemple
```
2026-01-12 21:14:03,SHOT,0.132,0,
2026-01-12 21:14:07,LOOT,3.84,2,Animal Oil Residue
2026-01-12 21:14:07,KILL,1,0,Exarosaur Young
```

---

## 🛠 Compilation

### Linux
```
make
./bin/tracker_modulaire
```

### Windows (MinGW)
```
make win
```

### Debug
```
make debug
```

### Release
```
make release
```

---

## 🚀 Extensions possibles

- Tracker mining
- Tracker sweat
- Export JSON
- Analyse multi-sessions
- Visualisation externe (Python / Grafana)
- GUI (plus tard)

---

## 🧩 Philosophie du projet

- 1 module = 1 responsabilité
- Pas de logique métier dans l’UI
- Parsing testable
- Stats testables
- Code portable
- Facile à relire dans 6 mois

---

## 👤 Auteur

**Megnoux Xavier**

Projet conçu et structuré avec une approche **modulaire, robuste et évolutive**, pour Entropia Universe, en **C pur**, esprit **low-level propre**.

---

## 📜 Licence

Ce projet est sous **licence propriétaire restrictive**.
Toute utilisation, copie ou redistribution sans autorisation est interdite.
Voir le fichier `LICENSE`.


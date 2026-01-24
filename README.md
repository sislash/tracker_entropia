# Tracker Modulaire – Entropia Universe

## 📌 Présentation

**Tracker Modulaire** est un programme en **C pur (C99)** destiné aux joueurs de **Entropia Universe**.
Il analyse le fichier `chat.log` du jeu afin de suivre précisément l’activité de chasse, aussi bien en **temps réel (LIVE)** qu’en **relecture (REPLAY)**.

Le programme permet notamment de :

- détecter automatiquement les actions de chasse (shots, loots, kills)
- enregistrer les événements dans un **CSV structuré et persistant**
- calculer des **statistiques détaillées de session** (loot, dépenses, net, return, profit)
- utiliser des **armes entièrement configurables** via `armes.ini`
- proposer une **interface console interactive**, pédagogique et portable **Linux & Windows**

Le projet est conçu de manière **totalement modulaire**, robuste face aux variations du `chat.log`, et pensé pour évoluer (mining, sweat, autres trackers).

---

## 🎯 Objectifs du projet

- Séparer clairement **parsing / règles / calcul / affichage**
- Garantir des calculs fiables et reproductibles
- Être **portable Linux / Windows** sans modification du code
- Respecter une philosophie de **C propre** (responsabilités claires)
- Faciliter l’ajout de nouvelles règles, armes ou trackers

---

## 🧠 Principe de fonctionnement global

```
chat.log (Entropia)
        ↓
[ parser_engine ]   ← LIVE / REPLAY
        ↓
[ hunt_rules ]      → SHOT / LOOT / KILL
        ↓
hunt_log.csv        (persistant)
        ↓
[ tracker_stats ]   → calculs purs
        ↓
[ tracker_view ]    → dashboard console
```

---

## 📂 Arborescence du projet

```
tracker_modulaire/
├── bin/                    # Exécutables Linux & Windows
├── build/
│   ├── src/                # Fichiers objets Linux (.o)
│   └── win/                # Fichiers objets Windows (.o)
├── include/                # Headers (.h)
├── logs/                   # Fichiers runtime
│   ├── hunt_log.csv        # CSV principal
│   ├── hunt_session.offset # Offset de session
│   └── weapon_selected.txt # Arme active
├── src/                    # Sources (.c)
├── tests/
│   └── hunt_rules_cases.txt
├── armes.ini               # Configuration des armes
├── Makefile
├── README.md
└── LICENSE
```

---

## 🔫 Configuration des armes – `armes.ini`

Le fichier `armes.ini` permet de définir **toutes les armes utilisées pour le calcul des coûts**.
Aucune recompilation n’est nécessaire : il suffit de modifier ce fichier.

Chaque arme représente **un modèle de coût par tir**, utilisé lorsque le CSV ne contient pas de ligne de dépense explicite.

### 📄 Structure générale

```ini
[player]
name = NomDuJoueur

[Nom Exact de l'Arme]
dpp = 2.84
ammo_shot = 0.04000
decay_shot = 0.01234
amp_decay_shot = 0.00456
markup = 1.02
notes = Arme courte portée, low level
```

### 🔎 Signification des champs

- `dpp` : Damage Per PEC (informatif, affichage)
- `ammo_shot` : coût de munition par tir (PED)
- `decay_shot` : decay de l’arme par tir (PED)
- `amp_decay_shot` : decay de l’amplificateur par tir (PED)
- `markup` : multiplicateur appliqué au coût total (ex: 1.02 = +2 %)
- `notes` : texte libre (optionnel)

---

## ➕ Comment ajouter une arme dans `armes.ini`

### Étape 1 : récupérer le nom exact de l’arme

⚠️ **Le nom de la section doit correspondre exactement au nom affiché dans Entropia Universe**.
C’est ce nom qui sera comparé à l’arme sélectionnée dans le menu.

Exemple correct :
```ini
[Breer P5a (L)]
```

Exemple incorrect :
```ini
[Breer P5a]
```

---

### Étape 2 : créer la section de l’arme

Ajoute une nouvelle section à la fin du fichier `armes.ini` :

```ini
[Breer P5a (L)]
dpp = 2.84
ammo_shot = 0.04000
decay_shot = 0.01200
amp_decay_shot = 0.00000
markup = 1.00
notes = Arme de test, sans ampli
```

---

### Étape 3 : vérifier les valeurs

- Toutes les valeurs sont exprimées en **PED**
- Les décimales sont importantes (utilise au moins 5 ou 6 décimales)
- `amp_decay_shot` peut être `0.0` si aucun ampli n’est utilisé
- `markup` doit être `1.0` si aucun MU n’est appliqué

---

### Étape 4 : recharger les armes dans le programme

Dans le programme :

1. Ouvre **Menu principal → Menu chasse**
2. Choisis **Recharger armes.ini**
3. Sélectionne l’arme via **Choisir une arme active**

Le coût par tir sera immédiatement utilisé pour les calculs.

---

### 🧠 Comment le coût par tir est calculé

```
cout_shot = (ammo_shot + decay_shot + amp_decay_shot) × markup
```

Ce coût est ensuite multiplié par le nombre de `SHOT` détectés.

---

## 🚀 Guide rapide – première utilisation (5 minutes)

⚠️ **Avant toute chose**, il est nécessaire de récupérer correctement le projet depuis GitHub.

### 0️⃣ Récupérer le projet depuis GitHub (Linux & Windows)

Le projet officiel est disponible ici :

```
https://github.com/sislash/tracker_entropia
```

---

### 🐧 Linux

#### Prérequis

- `git`
- `gcc`
- `make`

Sur Debian / Ubuntu :
```bash
sudo apt update
sudo apt install git build-essential
```

#### Clonage

```bash
git clone https://github.com/sislash/tracker_entropia.git
cd tracker_entropia
```

---

### 🪟 Windows

Sous Windows, **aucune compilation n’est obligatoire** :
➡️ **l’exécutable `.exe` est déjà fourni** dans le dépôt.

---

#### Option A – Utiliser directement l’exécutable (recommandé)

Après avoir cloné le dépôt (ou extrait le ZIP), tu trouveras :

```
bin/tracker_modulaire.exe
```

##### Démarrage correct du programme

⚠️ Il est **très important** de lancer le programme depuis le **dossier racine du projet**.

**Méthode simple (Explorateur Windows)** :
1. Ouvre le dossier `tracker_entropia`
2. Va dans le dossier `bin/`
3. Double-clique sur `tracker_modulaire.exe`

👉 Le programme utilisera automatiquement :
- `armes.ini`
- le dossier `logs/`
- les fichiers de session

**Méthode recommandée (Invite de commandes / PowerShell)** :

```powershell
cd path\to\tracker_entropia
.\bin\tracker_modulaire.exe
```

Cette méthode évite les problèmes de chemins relatifs.

---

#### Option B – Compiler soi-même (optionnel)

- Installer **MinGW-w64**
- Vérifier que `x86_64-w64-mingw32-gcc` est disponible

```bash
make win
```

L’exécutable sera généré dans `bin/`.

---

### 📁 Vérification de la structure

> 🔁 Rappel (si tu n’as pas encore cloné le dépôt ou si tu as un doute)

Après le clonage, tu dois avoir **au minimum** :

```
tracker_entropia/
├── src/
├── include/
├── tests/
├── armes.ini
├── Makefile
├── README.md
└── LICENSE
```

Si un de ces éléments manque, la compilation ou l’exécution ne fonctionneront pas correctement.

---

### 1️⃣ Compilation et lancement (Linux)

```bash
make
./bin/tracker_modulaire
```

Au premier lancement, le programme :
- crée automatiquement le dossier `logs/`
- initialise le fichier `hunt_log.csv`
- prépare le fichier d’offset de session

---

### 2️⃣ Vérifier / configurer les armes

- Ouvre le fichier `armes.ini`
- Vérifie que ton arme actuelle y est bien définie
- Lance le programme puis :
  - **Menu principal → Menu chasse**
  - **Recharger armes.ini**
  - **Choisir une arme active**

👉 Cette étape est importante pour que les **dépenses par tir** soient correctement calculées.

---

### 3️⃣ Lancer une session de chasse (LIVE)

- Dans le **menu chasse** :
  - Choisis **Démarrer LIVE**
- Le programme lit le `chat.log` en temps réel
- Chaque SHOT / LOOT / KILL est enregistré dans le CSV

---

### 4️⃣ Consulter les statistiques

- **Menu chasse → Afficher les stats**
- ou **Dashboard LIVE** pour un affichage auto-refresh

Les statistiques sont calculées **à partir de l’offset de session**.

---

### 5️⃣ Fin de session

- Arrête le parser
- Consulte les stats finales
- Optionnel : définir l’offset à la fin du CSV pour préparer une nouvelle session

---

## 🧪 Exemple de session complète (LIVE → stats → reset)

### Étape A : démarrage

1. Lancer le programme
2. Menu chasse → Démarrer LIVE
3. Chasser normalement (shots, kills, loots)

---

### Étape B : consultation des résultats

1. Menu chasse → Arrêter le parser
2. Menu chasse → Afficher les stats

Résultats visibles :
- loot total
- dépenses
- net (profit / perte)
- return (%)
- coût par tir

---

### Étape C : préparation d’une nouvelle session

**Option 1 – Continuer le même CSV**
- Menu chasse → Définir offset = fin actuelle du CSV
- Les prochaines stats repartiront de zéro visuellement

**Option 2 – Réinitialiser complètement**
- Menu principal → Vider le CSV
- L’offset est remis à 0
- Nouvelle session propre

---

## 📦 Description détaillée des modules

### 🔹 main.c
Point d’entrée du programme. Initialise l’environnement et lance le menu principal.

### 🔹 Menus & UI

- `menu_principale.c` : menu principal pédagogique et état global
- `menu_tracker_chasse.c` : menu chasse (armes, stats, dashboard)
- `ui_utils.c` : utilitaires console (clear, sleep, clavier, pause)

### 🔹 Parsing & règles

- `parser_engine.c` : lecture du chat.log (LIVE / REPLAY)
- `parser_thread.c` : exécution dans un thread dédié
- `hunt_rules.c` : analyse sémantique des lignes

### 🔹 Données & calculs

- `csv.c` : écriture CSV robuste
- `tracker_stats.c` : calculs purs (loot, dépenses, net, return)
- `tracker_view.c` : affichage console et dashboard

### 🔹 Session & chemins

- `session.c` : gestion de l’offset de session
- `core_paths.c` : centralisation des chemins
- `fs_utils.c` : filesystem portable

---

## 🧪 Tests

```bash
make test
```

Tests unitaires des règles de chasse via `hunt_rules_cases.txt`.
Objectif : **ne jamais casser le parsing**.

---

## ⚠️ Limitations connues

- Le parsing dépend strictement du format du `chat.log` d’Entropia Universe
- Application **console-only** (pas de GUI)
- Une seule session active à la fois

---

## 🧩 Philosophie du projet

- 1 module = 1 responsabilité
- Calculs séparés de l’affichage
- Code portable et lisible
- Pensé pour une maintenance long terme

---

## 👤 Auteur

**Megnoux Xavier**

---

## 📜 Licence

Ce projet est sous **licence propriétaire restrictive**.
Toute utilisation, copie ou redistribution sans autorisation est interdite.
Voir le fichier `LICENSE`.


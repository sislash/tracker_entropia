# Tracker Modulaire — Entropia Universe (WL / Fenêtre)

Programme **modulaire** en **C pur (C99)** pour **Entropia Universe**, version **UI en fenêtre** (Linux **X11**, Windows **Win32**).  
Il analyse le fichier `chat.log` du jeu afin de suivre une activité de chasse en **temps réel (LIVE)** ou en **relecture (REPLAY)**, en enregistrant les évènements dans des **CSV persistants** et en calculant des **statistiques de session** (loot, dépenses, net, return, tops, etc.).

Objectifs : code **portable Linux / Windows**, **robuste** face aux variations de logs, et facilement **extensible** (nouveaux trackers / nouvelles règles).

---

## Sommaire
- [Fonctionnalités](#fonctionnalités)
- [Compatibilité](#compatibilité)
- [Arborescence](#arborescence)
- [Compilation](#compilation)
- [Démarrage rapide](#démarrage-rapide)
- [Utilisation](#utilisation)
- [Configuration](#configuration)
  - [Chemin du chat.log](#chemin-du-chatlog)
  - [armes.ini](#armesini)
  - [markup.ini](#markupini)
  - [Option Sweat](#option-sweat)
- [Fichiers générés](#fichiers-générés)
- [Format CSV](#format-csv)
- [Tests](#tests)
- [Dépannage](#dépannage)
- [Architecture](#architecture)
- [Licence](#licence)

---

## Fonctionnalités

### Tracker CHASSE
- Lecture du `chat.log` en **LIVE** / **REPLAY**
- Détection d’évènements (selon les règles de parsing) :
  - `SHOT`, `KILL`, `LOOT` (loot multi-lignes groupés), etc.
- Statistiques de session :
  - shots, kills
  - loot total (PED)
  - dépenses :
    - **logguées** (si présentes dans le `chat.log`)
    - ou **estimées** via le coût/tir de l’arme active (`armes.ini`)
  - net / return
  - top mobs (kills)
- Interface **fenêtre** (dashboard / pages / menus)

### Tracker GLOBALS (mobs + craft)
- Parsing séparé + CSV séparé
- Dashboard dédié
- Comptage / agrégation des globals (mobs + craft) et tops

### Robustesse
- Création automatique du dossier `logs/`
- CSV initialisés avec header si nécessaire
- Persistance des réglages (offset session, arme active, options)

---

## Compatibilité
- **Linux** : GCC + Make + **X11**
- **Windows** :
  - compilation via **MinGW-w64** (cross ou natif)
  - exécutable `.exe` généré dans `bin/`

Standard : **C99**

> Linux : nécessite le link avec X11 (`-lX11`) + math (`-lm`).

---

## Arborescence

Version WL (fenêtre) :

    tracker_modulaire_fenetre_WL_V2/
    ├── build/                  # Objets (.o)
    │   └── win/                # Objets Windows (.o) si compilation win
    ├── bin/                    # Exécutables Linux & Windows
    ├── include/                # Headers (.h)
    ├── src/                    # Sources (.c)
    ├── armes.ini               # Config armes
    ├── markup.ini              # Config MU loot (optionnel)
    ├── Makefile
    └── LICENSE

Dossiers runtime (créés automatiquement) :

    logs/                       # CSV + fichiers persistants

---

## Compilation

### Linux
Prérequis : `gcc`, `make`, **X11 dev**

Exemples selon distro :
- Debian/Ubuntu : `sudo apt install build-essential libx11-dev`
- Fedora : `sudo dnf install gcc make libX11-devel`
- Arch : `sudo pacman -S gcc make libx11`

Build + run :

    make
    ./bin/app_fenetre

Raccourci :

    make run

### Windows
Compilation via MinGW-w64 (cross-compile le plus courant) :

    make win

Sortie :

    bin/app_fenetre.exe

Important : lancer depuis la **racine** du projet (chemins relatifs vers `armes.ini`, `markup.ini`, `logs/`).

PowerShell :

    cd path\to\tracker_modulaire_fenetre_WL_V2
    .\bin\app_fenetre.exe

---

## Démarrage rapide

1. Vérifie que `armes.ini` existe et contient ton arme (voir [armes.ini](#armesini))
2. Lance le programme
3. Menu CHASSE :
   - sélectionne l’arme active
   - démarre le parser en LIVE
4. Chasse normalement
5. Arrête le parser pour obtenir les stats et (optionnel) exporter la session

---

## Utilisation

### LIVE
- Le parser lit le `chat.log` en continu
- Les évènements sont ajoutés au CSV
- Le dashboard permet de consulter l’état et les stats

### REPLAY
- Lecture du `chat.log` “comme un fichier”
- Utile pour recalculer / rejouer une période (selon options du menu)

### Fin de session (CHASSE)
Le menu propose une action “arrêt” qui :
- stoppe le parser
- calcule les stats depuis l’offset courant
- exporte un résumé dans `logs/sessions_stats.csv` (si activé par le menu)
- met à jour l’offset (pour repartir propre sur la prochaine session)

---

## Configuration

### Chemin du chat.log

Le programme tente de trouver automatiquement le `chat.log`.  
Pour une configuration **robuste**, tu peux forcer le chemin via variable d’environnement :

Linux / Wine :

    export ENTROPIA_CHATLOG="/chemin/vers/chat.log"
    ./bin/app_fenetre

Windows (PowerShell) :

    $env:ENTROPIA_CHATLOG="C:\...\Entropia Universe\chat.log"
    .\bin\app_fenetre.exe

---

### armes.ini

Le fichier `armes.ini` définit le **coût par tir** utilisé quand le log ne fournit pas de dépenses explicites.

Règle de base :

    cout_shot = (ammo_shot + decay_shot + amp_decay_shot) × markup

Le coût par session est ensuite :

    cost_total = cout_shot × nombre_de_SHOT

#### Structure minimale (exemple)

    [PLAYER]
    name = NomDuJoueur

    [Breer P5a (L)]
    dpp = 2.84
    ammo_shot = 0.04000
    decay_shot = 0.01234
    amp_decay_shot = 0.00456
    markup = 1.02
    notes = Texte libre (optionnel)

Notes importantes :
- Les valeurs sont en **PED**
- Utilise des décimales (5–6 décimales conseillé)
- Le nom de section de l’arme doit correspondre exactement à celui utilisé dans le menu (et idéalement celui du jeu)

#### Sélection de l’arme active
Menu CHASSE → “Choisir une arme active”  
Persistée dans :

    logs/weapon_selected.txt

---

### markup.ini

Optionnel : permet d’estimer un loot **TT vs MU**.

Format :
- section = nom exact de l’item
- `type` :
  - `percent` : multiplicateur (ex: `1.10` = 110%)
  - `tt_plus` : ajoute une valeur fixe TT (PED)
- `value` : valeur associée au type

Exemple :

    [Shrapnel]
    type = percent
    value = 1.10

    [Animal Oil Residue]
    type = percent
    value = 1.025

Si `markup.ini` est absent ou incomplet :
- fallback : MU = TT (multiplicateur 1.00)

---

### Option Sweat

Option dédiée au suivi du **Vibrant Sweat** (sweating) :
- total de sweat
- nombre d’extractions
- moyenne par extraction

Activation / désactivation via le menu CHASSE.  
Persistée dans :

    logs/options.cfg

Exemples :

    sweat_tracker=1
    sweat_tracker=0

Quand OFF :
- les lignes “Vibrant Sweat” sont ignorées
- aucune stat sweat n’est calculée

---

## Fichiers générés

Dans `logs/` :

- `hunt_log.csv` : évènements de chasse
- `globals.csv` : évènements globals (mobs + craft)
- `hunt_session.offset` : offset de session (stats calculées à partir de cette position)
- `weapon_selected.txt` : arme active
- `options.cfg` : options (ex: sweat)
- `sessions_stats.csv` : export de résumé de session (si utilisé)

---

## Format CSV

CSV “simple et robuste” : les virgules / retours lignes des champs sont neutralisés.

Header (6 colonnes) :

    timestamp,type,target_or_item,qty,value,raw

Champs :
- `timestamp` : date/heure
- `type` : évènement (`SHOT`, `KILL`, `LOOT`, ...)
- `target_or_item` : mob / item
- `qty` : quantité (si applicable)
- `value` : valeur (si applicable)
- `raw` : ligne source (trace/debug)

---

## Tests

Le README historique contient une section tests.  
⚠️ **Dans cette version WL (fenêtre), il n’y a pas de dossier `tests/` ni de cible `make test` par défaut.**

Si tu veux réactiver des tests unitaires des règles de parsing :
- ajoute un dossier `tests/`
- ajoute une cible `test` dans le Makefile (ou un runner minimal)
- garde un fichier de cas (ex: `tests/hunt_rules_cases.txt`)

Objectif : garantir que l’ajout de nouvelles règles ne casse pas les patterns existants.

---

## Dépannage

### Le programme ne trouve pas chat.log
- utilise `ENTROPIA_CHATLOG` pour forcer le chemin
- vérifie que le client EU écrit bien le log (option “log to file” côté jeu)

### Dépenses à 0
- aucune dépense n’est logguée dans le chat, et aucune arme active n’est sélectionnée
- ou l’arme active n’existe pas (nom de section exact) dans `armes.ini`

### Linux : erreur `X11/Xlib.h` introuvable ou link `-lX11`
- installe les dépendances X11 dev (ex: `libx11-dev` / `libX11-devel`)
- puis :

    make clean
    make

### Windows : l’exe ne marche pas si lancé depuis bin/
- lance depuis la racine du projet
- ou assure que `armes.ini`, `markup.ini` et `logs/` sont accessibles depuis le répertoire courant

---

## Architecture

Principe :

    chat.log (Entropia)
            ↓
    [ parser_engine ]   ← LIVE / REPLAY
            ↓
    [ rules ]           → SHOT / LOOT / KILL / GLOBALS / SWEAT
            ↓
    CSV (persistants)
            ↓
    [ stats ]           → calculs purs
            ↓
    [ view / menus ]    → UI fenêtre (Linux X11 / Windows Win32)

Philosophie :
- 1 module = 1 responsabilité
- calculs séparés de l’affichage
- code portable et lisible
- maintenance long terme

---

## Licence
Licence propriétaire restrictive. Voir le fichier `LICENSE`.

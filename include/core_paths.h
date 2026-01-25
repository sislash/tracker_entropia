#ifndef CORE_PATHS_H
# define CORE_PATHS_H

# include <stddef.h>

/*
** Tracker Modulaire - core_paths
**
** Centralise les chemins de fichiers/dossiers.
**
** Règle: aucun autre module ne doit hardcoder "logs/...".
*/

# define TM_DIR_LOGS               "logs"
# define TM_FILE_HUNT_CSV          "logs/hunt_log.csv"
# define TM_FILE_SESSION_OFFSET    "logs/hunt_session.offset"
# define TM_FILE_WEAPON_SELECTED   "logs/weapon_selected.txt"
# define TM_FILE_ARMES_INI         "armes.ini"
# define TM_FILE_OPTIONS_CFG       "logs/options.cfg"
# define TM_FILE_SESSIONS_STATS_CSV "logs/sessions_stats.csv"
/* Globals / HOF / ATH (mob + craft) */
# define TM_FILE_GLOBALS_CSV "logs/globals.csv"

const char	*tm_path_globals_csv(void);
const char  *tm_path_sessions_stats_csv(void);
const char  *tm_path_options_cfg(void);
const char	*tm_path_hunt_csv(void);
const char	*tm_path_session_offset(void);
const char	*tm_path_weapon_selected(void);
const char	*tm_path_armes_ini(void);

int			 tm_ensure_logs_dir(void);

#endif

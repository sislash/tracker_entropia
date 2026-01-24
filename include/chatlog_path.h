#ifndef CHATLOG_PATH_H
# define CHATLOG_PATH_H

# include <stddef.h>

/*
** Construit le chemin vers chat.log en fonction des variables d'environnement.
**
** Windows: %USERPROFILE%\Documents\Entropia Universe\chat.log
** Linux:  $HOME/... (à adapter selon ton setup; par défaut $HOME/chat.log)
*/

int	chatlog_build_path(char *out, size_t outsz);

#endif

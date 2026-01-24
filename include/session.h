#ifndef SESSION_H
# define SESSION_H

/*
** Session: gestion d'un offset/point de départ pour les stats (hunt session).
*/

long	session_load_offset(const char *path);
int		session_save_offset(const char *path, long offset);

/*
** Compte le nombre de lignes de DONNEES dans un CSV (ignore l'en-tête si
** présente). Retourne 0 si fichier absent.
*/
long	session_count_data_lines(const char *csv_path);

#endif

#ifndef CSV_H
# define CSV_H

# include <stdio.h>

/*
 * * CSV helpers: écriture simple et split basique.
 **
 ** Hypothèse: champs sans virgules ni guillemets (format log interne).
 */

int		csv_split_n(char *line, char **out, int n);
void	csv_write_field(FILE *f, const char *s);
void	csv_write_row6(FILE *f,
					   const char *f0, const char *f1, const char *f2,
					const char *f3, const char *f4, const char *f5);

/*
 * * Ensure header exists in the CSV file.
 ** Writes: "timestamp,type,target_or_item,qty,value,raw\n" if file is empty.
 */
void	csv_ensure_header6(FILE *f);

#endif

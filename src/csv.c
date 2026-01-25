#include "csv.h"

#include <stdlib.h>

void csv_ensure_header6(FILE *f)
{
	long endpos;
	
	if (!f)
		return;
	
	/* on va à la fin pour savoir si fichier vide */
	fseek(f, 0, SEEK_END);
	endpos = ftell(f);
	if (endpos == 0)
		fprintf(f, "timestamp,type,target_or_item,qty,value,raw\n");
	
	/* on revient à la fin (append) */
	fseek(f, 0, SEEK_END);
}

int	csv_split_n(char *line, char **out, int n)
{
	int		i;
	char	*p;

	if (!line || !out || n <= 0)
		return (0);
	
	i = 0;
	p = line;
	while (i < n)
	{
		out[i] = p;
		while (*p && *p != ',' && *p != '\n' && *p != '\r')
			p++;
		if (*p == ',')
		{
			*p = '\0';
			p++;
			i++;
			continue;
		}
		if (*p)
			*p = '\0';
		i++; /* on compte le champ courant */
		break;
	}
	return (i);
}

void	csv_write_field(FILE *f, const char *s)
{
	if (!f)
		return;
	if (!s)
		s = "";
	while (*s)
	{
		if (*s == ',' || *s == '\n' || *s == '\r')
			fputc(' ', f);
		else
			fputc(*s, f);
		s++;
	}
}

void	csv_write_row6(FILE *f,
			const char *f0, const char *f1, const char *f2,
			const char *f3, const char *f4, const char *f5)
{
	csv_write_field(f, f0);
	fputc(',', f);
	csv_write_field(f, f1);
	fputc(',', f);
	csv_write_field(f, f2);
	fputc(',', f);
	csv_write_field(f, f3);
	fputc(',', f);
	csv_write_field(f, f4);
	fputc(',', f);
	csv_write_field(f, f5);
	fputc('\n', f);
}

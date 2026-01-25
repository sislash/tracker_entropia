#include "hunt_rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Format du fichier de tests (une ligne par cas):
 *   <CHAT_LINE>|||<TYPE>|||<NAME>|||<QTY>|||<VALUE>|||<PENDING_TYPE>
 *
 * - Les lignes vides et celles qui commencent par '#' sont ignorées.
 * - TYPE = "-" signifie: aucun event attendu (hunt_parse_line() == -1).
 * - PENDING_TYPE = "-" ou vide: aucun pending event attendu.
 */

static void	rtrim(char *s)
{
	size_t	n;

	if (!s)
		return ;
	n = strlen(s);
	while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' '))
		s[--n] = '\0';
}

static int	split6(char *s, char *out[6])
{
	int		k;
	char	*p;

	for (k = 0; k < 6; ++k)
		out[k] = NULL;
	k = 0;
	out[k++] = s;
	while ((p = strstr(s, "|||")) != NULL && k < 6)
	{
		*p = '\0';
		p[1] = '\0';
		p[2] = '\0';
		s = p + 3;
		out[k++] = s;
	}
	return (k);
}

static int	streq(const char *a, const char *b)
{
	if (!a)
		a = "";
	if (!b)
		b = "";
	return (strcmp(a, b) == 0);
}

static int	value_equal(const char *got, const char *exp)
{
	double	g;
	double	e;
	double	d;

	if (!exp || !*exp)
		return (1);
	if (!got)
		got = "";
	g = atof(got);
	e = atof(exp);
	d = g - e;
	if (d < 0)
		d = -d;
	return (d <= 0.00011);
}

int	main(int argc, char **argv)
{
	FILE		*fp;
	char		buf[4096];
	int		line_no;
	int		fails;

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <cases_file>\n", argv[0]);
		return (2);
	}
	fp = fopen(argv[1], "r");
	if (!fp)
	{
		perror("open cases file");
		return (2);
	}
	line_no = 0;
	fails = 0;
	while (fgets(buf, sizeof(buf), fp))
	{
		char		*parts[6];
		t_hunt_event	ev;
		t_hunt_event	pend;
		int		ret;
		int		has_pend;

		line_no++;
		rtrim(buf);
		if (buf[0] == '\0' || buf[0] == '#')
			continue ;
		if (split6(buf, parts) < 2)
		{
			fprintf(stderr, "[%d] bad format (need at least line|||TYPE)\n", line_no);
			fails++;
			continue ;
		}
		ret = hunt_parse_line(parts[0], &ev);
		has_pend = hunt_pending_pop(&pend);
		if (streq(parts[1], "-"))
		{
			if (ret != -1)
			{
				fprintf(stderr, "[%d] expected NO EVENT, got type='%s'\n",
					line_no, ev.type);
				fails++;
			}
			if (has_pend)
			{
				fprintf(stderr, "[%d] expected NO PENDING, got '%s'\n",
					line_no, pend.type);
				fails++;
			}
			continue ;
		}
		if (ret < 0)
		{
			fprintf(stderr, "[%d] expected event type='%s', got NO EVENT\n",
				line_no, parts[1]);
			fails++;
			continue ;
		}
		if (!streq(ev.type, parts[1]))
		{
			fprintf(stderr, "[%d] type mismatch: got '%s' expected '%s'\n",
				line_no, ev.type, parts[1]);
			fails++;
		}
		if (parts[2] && *parts[2] && !streq(ev.name, parts[2]))
		{
			fprintf(stderr, "[%d] name mismatch: got '%s' expected '%s'\n",
				line_no, ev.name, parts[2]);
			fails++;
		}
		if (parts[3] && *parts[3] && !streq(ev.qty, parts[3]))
		{
			fprintf(stderr, "[%d] qty mismatch: got '%s' expected '%s'\n",
				line_no, ev.qty, parts[3]);
			fails++;
		}
		if (parts[4] && *parts[4] && !value_equal(ev.value, parts[4]))
		{
			fprintf(stderr, "[%d] value mismatch: got '%s' expected '%s'\n",
				line_no, ev.value, parts[4]);
			fails++;
		}
		if (parts[5] && *parts[5] && !streq(parts[5], "-"))
		{
			if (!has_pend)
			{
				fprintf(stderr, "[%d] pending expected '%s' got NONE\n",
					line_no, parts[5]);
				fails++;
			}
			else if (!streq(pend.type, parts[5]))
			{
				fprintf(stderr, "[%d] pending type mismatch: got '%s' expected '%s'\n",
					line_no, pend.type, parts[5]);
				fails++;
			}
		}
		else
		{
			if (has_pend)
			{
				fprintf(stderr, "[%d] pending NONE expected, got '%s'\n",
					line_no, pend.type);
				fails++;
			}
		}
	}
	fclose(fp);
	if (fails)
	{
		fprintf(stderr, "FAIL: %d case(s)\n", fails);
		return (1);
	}
	printf("OK\n");
	return (0);
}

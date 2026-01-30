#include "hunt_rules.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE 2048

/*
 * NOTE (portabilité / -Werror):
 *  - Sur certaines toolchains (ex: MinGW), strncpy() déclenche des warnings
 *    "output may be truncated" qui cassent la build avec -Werror.
 *  - On utilise donc une copie sûre basée sur snprintf(), qui garantit
 *    la terminaison NUL.
 */

/*
 * Grouping LOOT -> KILL:
 *  - Plusieurs lignes "You received" peuvent appartenir au MÊME kill.
 *  - On groupe les loots proches (fenêtre en secondes) -> 1 seul KILL.
 *  - On évite aussi de créer des kills "hors combat" (craft/rewards/etc.)
 */
#define LOOT_GROUP_WINDOW_SEC 5
#define COMBAT_TO_LOOT_WINDOW_SEC 60

static void safe_copy(char *dst, size_t dstsz, const char *src)
{
	size_t n;
	
	if (!dst || dstsz == 0)
		return;
	if (!src)
	{
		dst[0] = '\0';
		return;
	}
	n = strlen(src);
	if (n >= dstsz)
		n = dstsz - 1;
	memcpy(dst, src, n);
	dst[n] = '\0';
}

/* ========================================================================== */
/* Utils                                                                      */
/* ========================================================================== */

static void	chomp(char *s)
{
	size_t		n;
	
	if (!s)
		return ;
	n = strlen(s);
	while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
		s[--n] = '\0';
}

static void	now_timestamp(char *buf, size_t bufsz)
{
	time_t		t;
	struct tm	*lt;
	
	if (!buf || bufsz == 0)
		return ;
	t = time(NULL);
	lt = localtime(&t);
	if (!lt)
	{
		buf[0] = '\0';
		return ;
	}
	strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S", lt);
}

static int	is_2digits(const char *p)
{
	return (p && isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]));
}

static int	is_4digits(const char *p)
{
	return (p
	&& isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1])
	&& isdigit((unsigned char)p[2]) && isdigit((unsigned char)p[3]));
}

/* Format attendu: "YYYY-MM-DD HH:MM:SS" au début de ligne */
static int	extract_chatlog_timestamp(const char *line, char *out, size_t outsz)
{
	if (!line || strlen(line) < 19 || !out || outsz < 20)
		return (0);
	if (!is_4digits(line) || line[4] != '-' || !is_2digits(line + 5)
		|| line[7] != '-' || !is_2digits(line + 8) || line[10] != ' '
		|| !is_2digits(line + 11) || line[13] != ':' || !is_2digits(line + 14)
		|| line[16] != ':' || !is_2digits(line + 17))
		return (0);
	snprintf(out, outsz, "%.19s", line);
	return (1);
}

/* Convertit "YYYY-MM-DD HH:MM:SS" -> time_t (portable Win/Linux via mktime) */
static time_t	parse_ts_to_time(const char *ts)
{
	struct tm	tm;
	int			yr;
	int			mo;
	int			da;
	int			ho;
	int			mi;
	int			se;
	
	if (!ts || strlen(ts) < 19)
		return ((time_t)0);
	memset(&tm, 0, sizeof(tm));
	if (sscanf(ts, "%d-%d-%d %d:%d:%d", &yr, &mo, &da, &ho, &mi, &se) != 6)
		return ((time_t)0);
	tm.tm_year = yr - 1900;
	tm.tm_mon = mo - 1;
	tm.tm_mday = da;
	tm.tm_hour = ho;
	tm.tm_min = mi;
	tm.tm_sec = se;
	tm.tm_isdst = -1;
	return (mktime(&tm));
}

/* enlève seulement un '.' final, pas les décimales */
static void	trim_final_dot(char *s)
{
	size_t	n;
	
	if (!s)
		return ;
	n = strlen(s);
	while (n && (s[n - 1] == ' ' || s[n - 1] == '\t'))
		s[--n] = '\0';
	if (n && s[n - 1] == '.')
		s[n - 1] = '\0';
}

static int	contains_any(const char *line, const char *const *patterns, size_t n)
{
	size_t	i;
	
	if (!line)
		return (0);
	i = 0;
	while (i < n)
	{
		if (patterns[i] && strstr(line, patterns[i]))
			return (1);
		i++;
	}
	return (0);
}

/* renvoie un pointeur sur la fin du token trouvé, sinon NULL */
static const char	*find_after_token(const char *line, const char *const *tokens,
									  size_t n)
{
	size_t		i;
	const char	*p;
	
	if (!line)
		return (NULL);
	i = 0;
	while (i < n)
	{
		if (tokens[i])
		{
			p = strstr(line, tokens[i]);
			if (p)
				return (p + strlen(tokens[i]));
		}
		i++;
	}
	return (NULL);
}

/* strtod qui accepte aussi la virgule en remplaçant localement ',' -> '.' */
static double	strtod_comma_ok(const char *s)
{
	char	tmp[64];
	size_t	i;
	char	*e;
	
	if (!s)
		return (0.0);
	i = 0;
	while (s[i] && i < sizeof(tmp) - 1)
	{
		unsigned char c = (unsigned char)s[i];
		if (!(isdigit(c) || c == '.' || c == ',' || c == '-' || c == '+'))
			break ;
		tmp[i] = (c == ',') ? '.' : (char)c;
		i++;
	}
	tmp[i] = '\0';
	e = NULL;
	return (strtod(tmp, &e));
}

static void	zero_event(t_hunt_event *ev)
{
	if (!ev)
		return ;
	memset(ev, 0, sizeof(*ev));
}

/* ========================================================================== */
/* Pending event (ex: LOOT_ITEM + KILL)                                       */
/* ========================================================================== */

static int			g_has_pending = 0;
static t_hunt_event	g_pending;

int	hunt_pending_pop(t_hunt_event *ev)
{
	if (!ev || !g_has_pending)
		return (0);
	*ev = g_pending;
	g_has_pending = 0;
	memset(&g_pending, 0, sizeof(g_pending));
	return (1);
}

/* ========================================================================== */
/* Rendre le nom du joueur accessible au parser                               */
/* ========================================================================== */

static char g_player_name[128];

void hunt_rules_set_player_name(const char *name)
{
	safe_copy(g_player_name, sizeof(g_player_name), name);
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

int	hunt_should_ignore_line(const char *line)
{
	if (!line || !*line)
		return (1);
	/* ignore tout ce qui est dans le canal Rookie / Débutant */
	if (strstr(line, "[Rookie]") != NULL)
		return (1);
	if (strstr(line, "[Débutant]") != NULL)
		return (1);
	
	/* Canaux communautaires / trade / social */
	if (strstr(line, "[#"))
		return (1);
	
	/* Tags spécifiques parasites signalés */
	if (strstr(line, "[ROCKtropia") != NULL)
		return (1);
	if (strstr(line, "[HyperStim") != NULL)
		return (1);
	
	return (0);
}

int	hunt_parse_line(const char *line_in, t_hunt_event *ev)
{
	char		line[MAX_LINE];
	char		ts[32];
	static char	last_kill_ts[32] = "";
	static time_t	last_loot_t = 0;
	static time_t	last_combat_t = 0;
	int			has_extra;
	
	if (!line_in || !ev)
		return (-1);
	zero_event(ev);
	safe_copy(line, sizeof(line), line_in);
	chomp(line);
	safe_copy(ev->raw, sizeof(ev->raw), line);
	/* timestamp prioritaire = chat.log ; fallback = maintenant */
	ts[0] = '\0';
	if (!extract_chatlog_timestamp(line, ts, sizeof(ts)))
		now_timestamp(ts, sizeof(ts));
	safe_copy(ev->ts, sizeof(ev->ts), ts);
	
	has_extra = 0;
	
	/* 1) SHOT (EN + FR)
	 * 
	 * IMPORTANT:
	 *  - Evite les faux positifs du type:
	 *    "Critical hit - Armor penetration! You took ..."
	 *  - Un "Critical hit" ne compte comme SHOT que si la ligne indique
	 *    clairement une attaque du joueur (ex: "You inflicted").
	 */
	{
		static const char *const shot_patterns_strict[] = {
			"You inflicted ",
			"Vous avez infligé ",
			"The target Dodged your attack",
			"The target Evaded your attack",
			"La cible a esquivé votre attaque",
			"La cible a évité votre attaque",
			"You missed",
			"Vous avez raté",
			"Vous manquez votre cible"
		};
		
		if (contains_any(line, shot_patterns_strict,
			sizeof(shot_patterns_strict) / sizeof(shot_patterns_strict[0])))
		{
			time_t t = parse_ts_to_time(ts);
			
			if (t)
				last_combat_t = t;
			safe_copy(ev->type, sizeof(ev->type), "SHOT");
			safe_copy(ev->qty, sizeof(ev->qty), "1");
			return (0);
		}
		/* Critical hit: valide seulement si on inflige des dégâts */
		if ((strstr(line, "Critical hit") || strstr(line, "Coup critique"))
			&& (strstr(line, "You inflicted ") || strstr(line, "Vous avez infligé ")))
		{
			time_t t = parse_ts_to_time(ts);
			
			if (t)
				last_combat_t = t;
			safe_copy(ev->type, sizeof(ev->type), "SHOT");
			safe_copy(ev->qty, sizeof(ev->qty), "1");
			return (0);
		}
	}
	
	/* 2) RECEIVED => LOOT_ITEM (+ KILL dédupliqué) EN + FR */
	{
		static const char *const recv_tokens[] = {
			"You received ",
			"Vous avez reçu "
		};
		static const char *const value_tokens[] = {
			" Value:",
			" Value :",
			" Valeur:",
			" Valeur :"
		};
		const char	*start;
		const char	*xpos;
		const char	*valp;
		size_t		i;
		
		start = find_after_token(line, recv_tokens,
								 sizeof(recv_tokens) / sizeof(recv_tokens[0]));
		if (start)
		{
			xpos = strstr(start, " x (");
			valp = NULL;
			i = 0;
			while (i < sizeof(value_tokens) / sizeof(value_tokens[0]))
			{
				const char *p = strstr(start, value_tokens[i]);
				if (p)
				{
					valp = p;
					break ;
				}
				i++;
			}
			if (xpos && valp && xpos < valp)
			{
				char	item[256];
				size_t	len;
				int		qty;
				double	ped;
				char	qtybuf[32];
				char	pedbuf[64];
				const char *vstart;
				
				len = (size_t)(xpos - start);
				while (len > 0 && isspace((unsigned char)start[len - 1]))
					len--;
				if (len >= sizeof(item))
					len = sizeof(item) - 1;
				memcpy(item, start, len);
				item[len] = '\0';
				qty = atoi(xpos + (int)strlen(" x ("));
				
				/* -------------------------------------------------------------------------
				 * SWEAT: "Vibrant Sweat" ne doit JAMAIS compter comme un KILL.
				 * On le sort du flux LOOT_ITEM -> pending KILL.
				 * QTY: 1..4 par extraction (selon le chat.log).
				 * ------------------------------------------------------------------------- */
				if (strcmp(item, "Vibrant Sweat") == 0)
				{
					/* On garde qty, value (0.0000), mais type dédié */
					snprintf(qtybuf, sizeof(qtybuf), "%d", qty);
					safe_copy(ev->type, sizeof(ev->type), "SWEAT");
					safe_copy(ev->name, sizeof(ev->name), "Vibrant Sweat");
					safe_copy(ev->qty, sizeof(ev->qty), qtybuf);
					safe_copy(ev->value, sizeof(ev->value), "0.0000");
					/* surtout : PAS de pending KILL */
					return (0);
				}
				
				vstart = valp;
				/* saute exactement le token Value/Valeur trouvé */
				i = 0;
				while (i < sizeof(value_tokens) / sizeof(value_tokens[0]))
				{
					const char *p = strstr(start, value_tokens[i]);
					if (p == valp)
					{
						vstart = p + strlen(value_tokens[i]);
						break ;
					}
					i++;
				}
				while (*vstart && isspace((unsigned char)*vstart))
					vstart++;
				ped = strtod_comma_ok(vstart);
				snprintf(qtybuf, sizeof(qtybuf), "%d", qty);
				snprintf(pedbuf, sizeof(pedbuf), "%.4f", ped);
				safe_copy(ev->type, sizeof(ev->type), "LOOT_ITEM");
				safe_copy(ev->name, sizeof(ev->name), item);
				safe_copy(ev->qty, sizeof(ev->qty), qtybuf);
				safe_copy(ev->value, sizeof(ev->value), pedbuf);
				
				/* -------------------------------------------------------------------------
				 * pending KILL:
				 *  - 1 kill par paquet de loot (fenêtre LOOT_GROUP_WINDOW_SEC)
				 *  - seulement si combat récent (COMBAT_TO_LOOT_WINDOW_SEC)
				 *  - On conserve last_kill_ts (legacy) mais on ne déduplique plus "à la seconde".
				 * ------------------------------------------------------------------------- */
				{
					time_t t = parse_ts_to_time(ts);
					
					if (t)
					{
						/* même paquet -> pas de kill */
						if (last_loot_t && (t - last_loot_t) <= LOOT_GROUP_WINDOW_SEC)
						{
							last_loot_t = t;
							return (0);
						}
						last_loot_t = t;
						
						/* combat récent -> créer kill */
						if (last_combat_t && (t - last_combat_t) <= COMBAT_TO_LOOT_WINDOW_SEC)
						{
							zero_event(&g_pending);
							safe_copy(g_pending.ts, sizeof(g_pending.ts), ts);
							safe_copy(g_pending.type, sizeof(g_pending.type), "KILL");
							safe_copy(g_pending.name, sizeof(g_pending.name), "UNKNOWN");
							safe_copy(g_pending.raw, sizeof(g_pending.raw), line);
							g_has_pending = 1;
							has_extra = 1;
							/* legacy: on met à jour last_kill_ts sans l'utiliser comme filtre principal */
							snprintf(last_kill_ts, sizeof(last_kill_ts), "%s", ts);
							return (has_extra);
						}
						/* loot hors combat -> pas de kill */
						return (0);
					}
				}
				
				/* fallback si pas de timestamp parsable -> comportement legacy */
				if (ts[0] && strcmp(last_kill_ts, ts) != 0)
				{
					zero_event(&g_pending);
					safe_copy(g_pending.ts, sizeof(g_pending.ts), ts);
					safe_copy(g_pending.type, sizeof(g_pending.type), "KILL");
					safe_copy(g_pending.name, sizeof(g_pending.name), "UNKNOWN");
					safe_copy(g_pending.raw, sizeof(g_pending.raw), line);
					g_has_pending = 1;
					has_extra = 1;
					snprintf(last_kill_ts, sizeof(last_kill_ts), "%s", ts);
				}
				return (has_extra);
			}
			/* Fallback: received mais format inattendu */
			{
				char	tmp[512];
				
				safe_copy(tmp, sizeof(tmp), start);
				trim_final_dot(tmp);
				safe_copy(ev->type, sizeof(ev->type), "RECEIVED_OTHER");
				safe_copy(ev->name, sizeof(ev->name), tmp);
				
				/* Même logique de grouping/anti-faux-kill que LOOT_ITEM */
				{
					time_t t = parse_ts_to_time(ts);
					
					if (t)
					{
						if (last_loot_t && (t - last_loot_t) <= LOOT_GROUP_WINDOW_SEC)
						{
							last_loot_t = t;
							return (0);
						}
						last_loot_t = t;
						if (last_combat_t && (t - last_combat_t) <= COMBAT_TO_LOOT_WINDOW_SEC)
						{
							zero_event(&g_pending);
							safe_copy(g_pending.ts, sizeof(g_pending.ts), ts);
							safe_copy(g_pending.type, sizeof(g_pending.type), "KILL");
							safe_copy(g_pending.name, sizeof(g_pending.name), "UNKNOWN");
							safe_copy(g_pending.raw, sizeof(g_pending.raw), line);
							g_has_pending = 1;
							has_extra = 1;
							snprintf(last_kill_ts, sizeof(last_kill_ts), "%s", ts);
							return (has_extra);
						}
						return (0);
					}
				}
				
				/* fallback legacy si ts non parsable */
				if (ts[0] && strcmp(last_kill_ts, ts) != 0)
				{
					zero_event(&g_pending);
					safe_copy(g_pending.ts, sizeof(g_pending.ts), ts);
					safe_copy(g_pending.type, sizeof(g_pending.type), "KILL");
					safe_copy(g_pending.name, sizeof(g_pending.name), "UNKNOWN");
					safe_copy(g_pending.raw, sizeof(g_pending.raw), line);
					g_has_pending = 1;
					has_extra = 1;
					snprintf(last_kill_ts, sizeof(last_kill_ts), "%s", ts);
				}
				return (has_extra);
			}
		}
	}
	
	/* 3) KILL explicite (EN + FR) */
	{
		static const char *const kill_tokens[] = {
			"You killed ",
			"Vous avez tué ",
			"Vous tuez "
		};
		const char	*start;
		char		mob[256];
		
		start = find_after_token(line, kill_tokens,
								 sizeof(kill_tokens) / sizeof(kill_tokens[0]));
		if (start)
		{
			safe_copy(mob, sizeof(mob), start);
			trim_final_dot(mob);
			if (ts[0] && strcmp(last_kill_ts, ts) != 0)
			{
				safe_copy(ev->type, sizeof(ev->type), "KILL");
				safe_copy(ev->name, sizeof(ev->name), mob);
				snprintf(last_kill_ts, sizeof(last_kill_ts), "%s", ts);
				return (0);
			}
			return (-1);
		}
	}
	
	return (-1);
}

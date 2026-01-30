#include "parser_engine.h"
#include "globals_parser.h"
#include "hunt_rules.h"
#include "csv.h"

#include "fs_utils.h"
#include "ui_utils.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

static char g_my_name[128] = "";

static void	log_engine_error(const char *ctx, const char *path)
{
	FILE *f = fopen("logs/parser_debug.log", "ab");
	if (!f)
		return;
	fprintf(f, "[ENGINE] %s failed: errno=%d (%s) path=[%s]\n",
			ctx, errno, strerror(errno), path ? path : "(null)");
	fclose(f);
}

static int	append_event(FILE *out, const t_hunt_event *ev)
{
	/*
	 * * Format CSV (6 champs): ts,type,name,qty,value,raw
	 ** Pour l'instant ts/name/qty/value sont vides (RAW minimal).
	 */
	if (!out || !ev)
		return (-1);
	csv_write_row6(out, ev->ts, ev->type, ev->name, ev->qty, ev->value, ev->raw);
	fflush(out); /* <-- IMPORTANT pour voir le CSV se remplir en live */
	return (0);
}

static void	safe_copy_engine(char *dst, size_t dstsz, const char *src)
{
	size_t	n;
	
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

/*
 * * Setter: permet de filtrer les globals sur TON nom,
 ** et synchronise le nom côté hunt_rules (si besoin).
 ** A appeler après lecture de armes.ini (PLAYER name=...).
 */
void	parser_engine_set_player_name(const char *name)
{
	safe_copy_engine(g_my_name, sizeof(g_my_name), name);
	hunt_rules_set_player_name(name);
}

static void	map_globals_to_hunt(t_hunt_event *dst, const t_globals_event *src)
{
	memset(dst, 0, sizeof(*dst));
	
	safe_copy_engine(dst->ts, sizeof(dst->ts), src->ts);
	safe_copy_engine(dst->name, sizeof(dst->name), src->name);
	safe_copy_engine(dst->value, sizeof(dst->value), src->value);
	safe_copy_engine(dst->raw, sizeof(dst->raw), src->raw);
	
	/* Mapping type: GLOB_* => GLOBAL, HOF_* => HOF, ATH_* => ATH */
	if (strncmp(src->type, "GLOB_", 5) == 0)
		safe_copy_engine(dst->type, sizeof(dst->type), "GLOBAL");
	else if (strncmp(src->type, "HOF_", 4) == 0)
		safe_copy_engine(dst->type, sizeof(dst->type), "HOF");
	else if (strncmp(src->type, "ATH_", 4) == 0)
		safe_copy_engine(dst->type, sizeof(dst->type), "ATH");
	else
		safe_copy_engine(dst->type, sizeof(dst->type), src->type);
	
	/* qty vide pour globals (ou "1" si tu préfères) */
	dst->qty[0] = '\0';
}

static int	process_line(FILE *out, const char *line)
{
	t_hunt_event	ev;
	t_globals_event	gev;
	int		ret;
	
	/*
	 * * 1) GLOBALS/HOF/ATH (via globals_parser)
	 ** On filtre ici pour ne garder QUE ceux qui contiennent ton nom.
	 */
	if (globals_parse_line(line, &gev) == 1)
	{
		/* filtre: on ne garde que les globals qui contiennent mon nom */
		if (g_my_name[0] && !strstr(gev.raw, g_my_name))
			return (0);
		
		map_globals_to_hunt(&ev, &gev);
		append_event(out, &ev);
		return (0);
	}
	
	/*
	 * * 2) HUNT (shot/loot/kill)
	 ** On ignore les canaux parasites uniquement pour HUNT.
	 */
	if (hunt_should_ignore_line(line))
		return (0);
	
	ret = hunt_parse_line(line, &ev);
	if (ret < 0)
		return (0);
	
	append_event(out, &ev);
	while (hunt_pending_pop(&ev))
		append_event(out, &ev);
	return (0);
}

int	parser_run_replay(const char *chatlog_path, const char *csv_path,
					  volatile int *stop_flag)
{
	FILE	*in;
	FILE	*out;
	char	buf[2048];
	
	in = fs_fopen_shared_read(chatlog_path);
	if (!in)
		return (log_engine_error("open chatlog", chatlog_path), -1);
	out = fopen(csv_path, "ab");
	if (!out)
		return (log_engine_error("open csv", csv_path), fclose(in), -1);
	
	csv_ensure_header6(out);
	fflush(out);
	#ifdef DEBUG
	printf("[DEBUG] CSV opened: %s\n", csv_path);
	#endif
	
	while (!stop_flag || (*stop_flag == 0))
	{
		if (!fgets(buf, sizeof(buf), in))
		{
			#ifdef DEBUG
			printf("[DEBUG] REPLAY fgets EOF\n");
			#endif
			break ;
		}
		#ifdef DEBUG
		printf("[DEBUG] LINE: %s", buf);
		#endif
		process_line(out, buf);
	}
	fclose(out);
	fclose(in);
	return (0);
}

int	parser_run_live(const char *chatlog_path, const char *csv_path,
					volatile int *stop_flag)
{
	#ifdef DEBUG
	printf("[DEBUG] LIVE enter stop_flag=%p val=%d\n",
		   (void*)stop_flag, stop_flag ? *stop_flag : -999);
	fflush(stdout);
	#endif
	
	FILE	*in;
	FILE	*out;
	char	buf[2048];
	long	last_pos;
	
	in = fs_fopen_shared_read(chatlog_path);
	if (!in)
	{
		log_engine_error("open chatlog", chatlog_path);
		return (-1);
	}
	
	out = fopen(csv_path, "ab");
	if (!out)
	{
		log_engine_error("open csv", csv_path);
		fclose(in);
		return (-1);
	}
	
	#ifdef DEBUG
	printf("[DEBUG] CSV opened: %s\n", csv_path);
	#endif
	csv_ensure_header6(out);
	fflush(out);
	
	fseek(in, 0, SEEK_END);
	last_pos = ftell(in);
	while (!stop_flag || *stop_flag == 0)
	{
		if (fgets(buf, sizeof(buf), in))
		{
			process_line(out, buf);
			last_pos = ftell(in);
		}
		else
		{
			long	sz;
			
			clearerr(in);
			sz = fs_file_size(chatlog_path);
			/*
			 * * Comme dans ton ancien parseur:
			 ** - si le fichier a rétréci => rotation/troncature => reopen
			 ** - si le fichier a grandi => on se repositionne sur last_pos
			 ** - sinon => on attend un peu
			 */
			if (sz >= 0 && sz < last_pos)
			{
				fclose(in);
				in = fs_fopen_shared_read(chatlog_path);
				if (!in)
				{
					/* le fichier est peut-être lock/rotating : on attend et on retente */
					ui_sleep_ms(250);
					continue;
				}
				fseek(in, 0, SEEK_END);
				last_pos = ftell(in);
			}
			else if (sz >= 0 && sz > last_pos)
			{
				fseek(in, last_pos, SEEK_SET);
			}
			else
			{
				ui_sleep_ms(200);
				fseek(in, last_pos, SEEK_SET);
			}
		}
	}
	fclose(out);
	fclose(in);
	
	#ifdef DEBUG
	printf("[DEBUG] LIVE exit stop_flag=%p val=%d\n",
		   (void*)stop_flag, stop_flag ? *stop_flag : -999);
	fflush(stdout);
	#endif
	
	return (0);
}

#include "parser_engine.h"
#include "hunt_rules.h"
#include "csv.h"

#include "fs_utils.h"
#include "ui_utils.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

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
	** Format CSV (6 champs): ts,type,name,qty,value,raw
	** Pour l'instant ts/name/qty/value sont vides (RAW minimal).
	*/
	if (!out || !ev)
		return (-1);
	csv_write_row6(out, ev->ts, ev->type, ev->name, ev->qty, ev->value, ev->raw);
	fflush(out); /* <-- IMPORTANT pour voir le CSV se remplir en live */
	return (0);
}

static int	process_line(FILE *out, const char *line)
{
	t_hunt_event	ev;
	int			ret;

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
	csv_ensure_header6(out);
	fflush(out);
#endif
	
	while (!stop_flag || *stop_flag == 0)
	{
		if (!fgets(buf, sizeof(buf), in)){
			printf("[DEBUG] REPLAY fgets EOF\n");
			break;
		}
#ifdef DEBUG
		printf("[DEBUG] LINE: %s", buf);
		process_line(out, buf);
#endif
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
	csv_ensure_header6(out);
	fflush(out);
#endif
	
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
			** Comme dans ton ancien parseur:
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

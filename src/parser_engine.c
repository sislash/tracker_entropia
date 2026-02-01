/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_engine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_engine.h"
#include "globals_parser.h"
#include "hunt_rules.h"
#include "csv.h"
#include "fs_utils.h"
#include "ui_utils.h"
#include "sweat_option.h"
#include "core_paths.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static char	g_my_name[128] = "";

static void	log_engine_error(const char *ctx, const char *path)
{
	FILE	*f;
	
	f = fopen("logs/parser_debug.log", "ab");
	if (!f)
		return ;
	fprintf(f, "[ENGINE] %s failed: errno=%d (%s) path=[%s]\n",
			ctx, errno, strerror(errno), path ? path : "(null)");
	fclose(f);
}

static void	safe_copy_engine(char *dst, size_t dstsz, const char *src)
{
	size_t	n;
	
	if (!dst || dstsz == 0)
		return ;
	if (!src)
	{
		dst[0] = '\0';
		return ;
	}
	n = strlen(src);
	if (n >= dstsz)
		n = dstsz - 1;
	memcpy(dst, src, n);
	dst[n] = '\0';
}

void	parser_engine_set_player_name(const char *name)
{
	safe_copy_engine(g_my_name, sizeof(g_my_name), name);
	hunt_rules_set_player_name(name);
}

static void	map_globals_to_hunt(t_hunt_event *dst,
								const t_globals_event *src)
{
	memset(dst, 0, sizeof(*dst));
	safe_copy_engine(dst->ts, sizeof(dst->ts), src->ts);
	safe_copy_engine(dst->name, sizeof(dst->name), src->name);
	safe_copy_engine(dst->value, sizeof(dst->value), src->value);
	safe_copy_engine(dst->raw, sizeof(dst->raw), src->raw);
	if (strncmp(src->type, "GLOB_", 5) == 0)
		safe_copy_engine(dst->type, sizeof(dst->type), "GLOBAL");
	else if (strncmp(src->type, "HOF_", 4) == 0)
		safe_copy_engine(dst->type, sizeof(dst->type), "HOF");
	else if (strncmp(src->type, "ATH_", 4) == 0)
		safe_copy_engine(dst->type, sizeof(dst->type), "ATH");
	else
		safe_copy_engine(dst->type, sizeof(dst->type), src->type);
	dst->qty[0] = '\0';
}

static int	append_event(FILE *out, const t_hunt_event *ev)
{
	if (!out || !ev)
		return (-1);
	csv_write_row6(out, ev->ts, ev->type, ev->name,
				   ev->qty, ev->value, ev->raw);
	fflush(out);
	return (0);
}

static int	try_process_globals(FILE *out, const char *line)
{
	t_globals_event	gev;
	t_hunt_event		ev;
	
	if (globals_parse_line(line, &gev) != 1)
		return (0);
	if (g_my_name[0] && !strstr(gev.raw, g_my_name))
		return (1);
	map_globals_to_hunt(&ev, &gev);
	append_event(out, &ev);
	return (1);
}

static void	flush_pending_hunt(FILE *out, t_hunt_event *ev)
{
	while (hunt_pending_pop(ev))
		append_event(out, ev);
}

static void	process_hunt(FILE *out, const char *line)
{
	t_hunt_event	ev;
	int			ret;
	int			sweat_enabled;

	if (hunt_should_ignore_line(line))
		return ;
	ret = hunt_parse_line(line, &ev);
	if (ret < 0)
		return ;
	if (strcmp(ev.type, "SWEAT") == 0)
	{
		sweat_enabled = 0;
		sweat_option_load(tm_path_options_cfg(), &sweat_enabled);
		if (!sweat_enabled)
			return ;
	}
	append_event(out, &ev);
	flush_pending_hunt(out, &ev);
}

static void	process_line(FILE *out, const char *line)
{
	if (try_process_globals(out, line))
		return ;
	process_hunt(out, line);
}

static int	open_io_files(FILE **in, FILE **out,
						  const char *chatlog_path, const char *csv_path)
{
	*in = fs_fopen_shared_read(chatlog_path);
	if (!*in)
	{
		log_engine_error("open chatlog", chatlog_path);
		return (-1);
	}
	*out = fopen(csv_path, "ab");
	if (!*out)
	{
		log_engine_error("open csv", csv_path);
		fclose(*in);
		*in = NULL;
		return (-1);
	}
	csv_ensure_header6(*out);
	fflush(*out);
	return (0);
}

static int	replay_loop(FILE *in, FILE *out, volatile int *stop_flag)
{
	char	buf[2048];
	
	while (!stop_flag || (*stop_flag == 0))
	{
		if (!fgets(buf, sizeof(buf), in))
			break ;
		process_line(out, buf);
	}
	return (0);
}

int	parser_run_replay(const char *chatlog_path, const char *csv_path,
					  volatile int *stop_flag)
{
	FILE	*in;
	FILE	*out;
	
	if (open_io_files(&in, &out, chatlog_path, csv_path) < 0)
		return (-1);
	replay_loop(in, out, stop_flag);
	fclose(out);
	fclose(in);
	return (0);
}

static void	reopen_if_rotated(FILE **in, const char *path, long *last_pos)
{
	long	sz;
	
	clearerr(*in);
	sz = fs_file_size(path);
	if (sz >= 0 && sz < *last_pos)
	{
		fclose(*in);
		*in = fs_fopen_shared_read(path);
		if (!*in)
		{
			ui_sleep_ms(250);
			return ;
		}
		fseek(*in, 0, SEEK_END);
		*last_pos = ftell(*in);
	}
	else if (sz >= 0 && sz > *last_pos)
		fseek(*in, *last_pos, SEEK_SET);
	else
	{
		ui_sleep_ms(200);
		fseek(*in, *last_pos, SEEK_SET);
	}
}

static void	live_loop(FILE *in, FILE *out, const char *path,
					  volatile int *stop_flag)
{
	char	buf[2048];
	long	last_pos;
	
	fseek(in, 0, SEEK_END);
	last_pos = ftell(in);
	while (!stop_flag || (*stop_flag == 0))
	{
		if (fgets(buf, sizeof(buf), in))
		{
			process_line(out, buf);
			last_pos = ftell(in);
		}
		else
			reopen_if_rotated(&in, path, &last_pos);
	}
}

int	parser_run_live(const char *chatlog_path, const char *csv_path,
					volatile int *stop_flag)
{
	FILE	*in;
	FILE	*out;
	
	if (open_io_files(&in, &out, chatlog_path, csv_path) < 0)
		return (-1);
	live_loop(in, out, chatlog_path, stop_flag);
	fclose(out);
	fclose(in);
	return (0);
}

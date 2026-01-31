/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   session_export.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login             ###   ########.fr      */
/*                                                                            */
/* ************************************************************************** */

#define _CRT_SECURE_NO_WARNINGS

#include "session_export.h"
#include "csv.h"

#include <stdio.h>
#include <string.h>

typedef struct s_session_bufs
{
    char	kills[64];
    char	shots[64];
    char	loot[64];
    char	exp[64];
    char	net[64];
    char	ret[64];
}	t_session_bufs;

static void	str_copy(char *dst, size_t dstsz, const char *src)
{
    if (!dst || dstsz == 0)
        return ;
    if (!src)
    {
        dst[0] = '\0';
        return ;
    }
    snprintf(dst, dstsz, "%s", src);
}

static void	init_out_ranges(char *out_start, char *out_end)
{
    if (out_start)
        out_start[0] = '\0';
    if (out_end)
        out_end[0] = '\0';
}

static int	is_hunt_csv_header(long line_index, const char *line)
{
    if (line_index != 0)
        return (0);
    if (!line)
        return (0);
    if (strstr(line, "timestamp") && strstr(line, "type"))
        return (1);
    return (0);
}

static int	extract_timestamp_field(char *line, char *out, size_t outsz)
{
    char	*comma;
    
    if (!line || !out || outsz == 0)
        return (0);
    comma = strchr(line, ',');
    if (!comma)
        return (0);
    *comma = '\0';
    str_copy(out, outsz, line);
    return (1);
}

static void	process_range_file(FILE *f, long start_offset,
                               char *out_start, size_t out_start_sz,
                               char *out_end, size_t out_end_sz)
{
    char	line[1024];
    long	line_index;
    int		first_data_seen;
    
    line_index = 0;
    first_data_seen = 0;
    while (fgets(line, sizeof(line), f))
    {
        if (is_hunt_csv_header(line_index, line))
        {
            line_index++;
            continue ;
        }
        if (line_index++ < start_offset)
            continue ;
        if (!extract_timestamp_field(line, out_end, out_end_sz))
            continue ;
        if (!first_data_seen)
        {
            first_data_seen = 1;
            str_copy(out_start, out_start_sz, out_end);
        }
    }
}


int	session_extract_range_timestamps(const char *hunt_csv_path,
                                     long start_offset, char *out_start, size_t out_start_sz,
                                     char *out_end, size_t out_end_sz)
{
    FILE	*f;
    
    init_out_ranges(out_start, out_end);
    if (!hunt_csv_path || start_offset < 0)
        return (0);
    f = fopen(hunt_csv_path, "rb");
    if (!f)
        return (0);
    process_range_file(f, start_offset, out_start, out_start_sz,
                       out_end, out_end_sz);
    fclose(f);
    return (1);
}

static void	ensure_sessions_header(FILE *f)
{
    long	endpos;
    
    if (!f)
        return ;
    fseek(f, 0, SEEK_END);
    endpos = ftell(f);
    if (endpos == 0)
    {
        fprintf(f, "session_start,session_end,weapon,kills,shots,");
        fprintf(f, "loot_ped,expense_ped,net_ped,return_pct\n");
    }
}

static double	safe_return_pct(double loot, double expense)
{
    if (expense <= 0.0)
        return (0.0);
    return ((loot / expense) * 100.0);
}

static void	build_session_bufs(const t_hunt_stats *s, t_session_bufs *b)
{
    snprintf(b->kills, sizeof(b->kills), "%ld", (long)s->kills);
    snprintf(b->shots, sizeof(b->shots), "%ld", (long)s->shots);
    snprintf(b->loot, sizeof(b->loot), "%.4f", s->loot_ped);
    snprintf(b->exp, sizeof(b->exp), "%.4f", s->expense_used);
    snprintf(b->net, sizeof(b->net), "%.4f", s->net_ped);
    snprintf(b->ret, sizeof(b->ret), "%.2f",
             safe_return_pct(s->loot_ped, s->expense_used));
}

static void	write_fields_list(FILE *f, const char **fields, int count)
{
    int	i;
    
    i = 0;
    while (i < count)
    {
        csv_write_field(f, fields[i]);
        if (i + 1 < count)
            fputc(',', f);
        else
            fputc('\n', f);
        i++;
    }
}

static void	write_sessions_row(FILE *f, const t_hunt_stats *s,
                               const char *start_ts, const char *end_ts)
{
    t_session_bufs	b;
    const char		*wname;
    const char		*fields[9];
    
    build_session_bufs(s, &b);
    wname = (s->weapon_name[0] ? s->weapon_name : "");
    fields[0] = start_ts;
    fields[1] = end_ts;
    fields[2] = wname;
    fields[3] = b.kills;
    fields[4] = b.shots;
    fields[5] = b.loot;
    fields[6] = b.exp;
    fields[7] = b.net;
    fields[8] = b.ret;
    write_fields_list(f, fields, 9);
}

int	session_export_stats_csv(const char *out_csv_path, const t_hunt_stats *s,
                             const char *session_start_ts, const char *session_end_ts)
{
    FILE		*f;
    const char	*start_ts;
    const char	*end_ts;
    
    if (!out_csv_path || !s)
        return (0);
    f = fopen(out_csv_path, "ab");
    if (!f)
        return (0);
    ensure_sessions_header(f);
    start_ts = (session_start_ts ? session_start_ts : "");
    end_ts = (session_end_ts ? session_end_ts : "");
    write_sessions_row(f, s, start_ts, end_ts);
    fclose(f);
    return (1);
}

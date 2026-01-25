#define _CRT_SECURE_NO_WARNINGS

#include "session_export.h"
#include "csv.h"

#include <stdio.h>
#include <string.h>

/* petit helper: copie sûre */
static void	str_copy(char *dst, size_t dstsz, const char *src)
{
    if (!dst || dstsz == 0)
        return;
    if (!src)
    {
        dst[0] = '\0';
        return;
    }
    snprintf(dst, dstsz, "%s", src);
}

/* Lit timestamp de la 1ère et dernière ligne de la “session” dans hunt_csv.
 * start_offset = nombre de lignes déjà consommées (stats_compute utilise pareil)
 *
 * out_start/out_end : ISO string si dispo, sinon "".
 */
int	session_extract_range_timestamps(const char *hunt_csv_path, long start_offset,
                                     char *out_start, size_t out_start_sz,
                                     char *out_end, size_t out_end_sz)
{
    FILE	*f;
    char	line[1024];
    long	line_index;
    int		first_data_seen;
    
    if (out_start) out_start[0] = '\0';
    if (out_end) out_end[0] = '\0';
    if (!hunt_csv_path || start_offset < 0)
        return (0);
    
    f = fopen(hunt_csv_path, "rb");
    if (!f)
        return (0);
    
    line_index = 0;
    first_data_seen = 0;
    while (fgets(line, sizeof(line), f))
    {
        /* skip header si présent */
        if (line_index == 0 && strstr(line, "timestamp") && strstr(line, "type"))
        {
            line_index++;
            continue;
        }
        
        /* on ignore les lignes avant start_offset */
        if (line_index < start_offset)
        {
            line_index++;
            continue;
        }
        
        /* timestamp = champ 0 avant la première virgule */
        {
            char *comma = strchr(line, ',');
            if (comma)
            {
                *comma = '\0';
                if (!first_data_seen)
                {
                    first_data_seen = 1;
                    str_copy(out_start, out_start_sz, line);
                }
                /* à chaque ligne, on met à jour out_end -> dernière ligne */
                str_copy(out_end, out_end_sz, line);
            }
        }
        line_index++;
    }
    fclose(f);
    return (1);
}

/* Header Excel-friendly + row with escaping via csv_write_field */
static void	ensure_sessions_header(FILE *f)
{
    long endpos;
    
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    endpos = ftell(f);
    if (endpos == 0)
    {
        fprintf(f,
                "session_start,session_end,weapon,kills,shots,"
                "loot_ped,expense_ped,net_ped,return_pct\n");
    }
}

static double	safe_return_pct(double loot, double expense)
{
    if (expense <= 0.0)
        return (0.0);
    return (loot / expense) * 100.0;
}

int	session_export_stats_csv(const char *out_csv_path,
                             const t_hunt_stats *s,
                             const char *session_start_ts,
                             const char *session_end_ts)
{
    FILE	*f;
    char	buf_kills[64];
    char	buf_shots[64];
    char	buf_loot[64];
    char	buf_exp[64];
    char	buf_net[64];
    char	buf_ret[64];
    double	expense;
    double	ret;
    
    if (!out_csv_path || !s)
        return (0);
    
    f = fopen(out_csv_path, "ab");
    if (!f)
        return (0);
    
    ensure_sessions_header(f);
    
    expense = s->expense_used;
    ret = safe_return_pct(s->loot_ped, expense);
    
    snprintf(buf_kills, sizeof(buf_kills), "%ld", (long)s->kills);
    snprintf(buf_shots, sizeof(buf_shots), "%ld", (long)s->shots);
    snprintf(buf_loot, sizeof(buf_loot), "%.4f", s->loot_ped);
    snprintf(buf_exp, sizeof(buf_exp), "%.4f", expense);
    snprintf(buf_net, sizeof(buf_net), "%.4f", s->net_ped);
    snprintf(buf_ret, sizeof(buf_ret), "%.2f", ret);
    
    csv_write_field(f, session_start_ts ? session_start_ts : "");
    fputc(',', f);
    csv_write_field(f, session_end_ts ? session_end_ts : "");
    fputc(',', f);
    csv_write_field(f, s->weapon_name[0] ? s->weapon_name : "");
    fputc(',', f);
    csv_write_field(f, buf_kills);
    fputc(',', f);
    csv_write_field(f, buf_shots);
    fputc(',', f);
    csv_write_field(f, buf_loot);
    fputc(',', f);
    csv_write_field(f, buf_exp);
    fputc(',', f);
    csv_write_field(f, buf_net);
    fputc(',', f);
    csv_write_field(f, buf_ret);
    fputc('\n', f);
    
    fclose(f);
    return (1);
}


/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_arme.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ###########       */
/*                                                                            */
/* ************************************************************************** */

#include "config_arme.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct s_parse_ctx
{
    arme_stats	cur;
    amp_stats	cur_amp;
    int			have_section;
    int			in_player;
    int			in_amp;
}	t_parse_ctx;

static void	str_rtrim(char *s)
{
    size_t	n;
    
    if (!s)
        return ;
    n = strlen(s);
    while (n > 0)
    {
        if (s[n - 1] == '\n' || s[n - 1] == '\r'
            || isspace((unsigned char)s[n - 1]))
            s[--n] = '\0';
        else
            break ;
    }
}

static char	*str_ltrim(char *s)
{
    if (!s)
        return (NULL);
    while (*s && isspace((unsigned char)*s))
        s++;
    return (s);
}

static int	line_is_comment_or_empty(const char *s)
{
    if (!s)
        return (1);
    while (*s && isspace((unsigned char)*s))
        s++;
    if (!*s)
        return (1);
    return (*s == ';' || *s == '#');
}

static int	is_value_char(int c)
{
    if (c == '+' || c == '-' || c == '.' || c == ',')
        return (1);
    if (c == 'e' || c == 'E')
        return (1);
    return (isdigit(c));
}

static int	parse_double_flex(const char *s, double *out)
{
    char	buf[128];
    size_t	i;
    char	*end;
    
    if (!s || !out)
        return (0);
    while (*s && isspace((unsigned char)*s))
        s++;
    i = 0;
    while (s[i] && i + 1 < sizeof(buf) && is_value_char((int)s[i]))
    {
        buf[i] = s[i];
        if (buf[i] == ',')
            buf[i] = '.';
        i++;
    }
    buf[i] = '\0';
    if (i == 0)
        return (0);
    *out = strtod(buf, &end);
    return (end != buf);
}

static void	safe_copy(char *dst, size_t dstsz, const char *src)
{
    size_t	i;
    
    if (!dst || dstsz == 0)
        return ;
    if (!src)
    {
        dst[0] = '\0';
        return ;
    }
    i = 0;
    while (src[i] && i + 1 < dstsz)
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void	arme_defaults(arme_stats *w)
{
    memset(w, 0, sizeof(*w));
    w->markup = 1.0;
    w->ammo_mu = 1.0;
    w->weapon_mu = 0.0;
    w->amp_mu = 0.0;
    w->amp_name[0] = '\0';
}

static void	amp_defaults(amp_stats *a)
{
    memset(a, 0, sizeof(*a));
    a->amp_mu = 1.0;
}

static int	db_amp_push(armes_db *db, const amp_stats *a)
{
    amp_stats	*tmp;
    
    tmp = (amp_stats *)realloc(db->amps.items,
                               (db->amps.count + 1) * sizeof(*db->amps.items));
    if (!tmp)
        return (0);
    db->amps.items = tmp;
    db->amps.items[db->amps.count] = *a;
    db->amps.count++;
    return (1);
}

static int	db_push(armes_db *db, const arme_stats *w)
{
    arme_stats	*tmp;
    
    tmp = (arme_stats *)realloc(db->items,
                                (db->count + 1) * sizeof(*db->items));
    if (!tmp)
        return (0);
    db->items = tmp;
    db->items[db->count] = *w;
    db->count++;
    return (1);
}

static const amp_stats	*find_amp(const armes_db *db, const char *name)
{
    size_t	i;
    
    if (!db || !name || !name[0])
        return (NULL);
    i = 0;
    while (i < db->amps.count)
    {
        if (strcmp(db->amps.items[i].name, name) == 0)
            return (&db->amps.items[i]);
        i++;
    }
    return (NULL);
}

static void	db_init(armes_db *db)
{
    db->items = NULL;
    db->count = 0;
    db->amps.items = NULL;
    db->amps.count = 0;
    db->player_name[0] = '\0';
}

static void	ctx_init(t_parse_ctx *ctx)
{
    arme_defaults(&ctx->cur);
    amp_defaults(&ctx->cur_amp);
    ctx->have_section = 0;
    ctx->in_player = 0;
    ctx->in_amp = 0;
}

static void	save_previous_section(armes_db *db, t_parse_ctx *ctx)
{
    if (!ctx->have_section)
        return ;
    if (ctx->in_amp)
        (void)db_amp_push(db, &ctx->cur_amp);
    else if (!ctx->in_player)
        (void)db_push(db, &ctx->cur);
}

static void	start_section(t_parse_ctx *ctx, const char *sec)
{
    ctx->have_section = 1;
    ctx->in_player = 0;
    ctx->in_amp = 0;
    if (strcmp(sec, "PLAYER") == 0)
        ctx->in_player = 1;
    else if (strncmp(sec, "AMP:", 4) == 0)
    {
        ctx->in_amp = 1;
        amp_defaults(&ctx->cur_amp);
        safe_copy(ctx->cur_amp.name, sizeof(ctx->cur_amp.name), sec + 4);
    }
    else
    {
        arme_defaults(&ctx->cur);
        safe_copy(ctx->cur.name, sizeof(ctx->cur.name), sec);
    }
}

static int	handle_section_line(armes_db *db, t_parse_ctx *ctx, char *p)
{
    char	*end;
    char	*sec;
    
    end = strchr(p, ']');
    if (!end)
        return (0);
    *end = '\0';
    save_previous_section(db, ctx);
    sec = p + 1;
    start_section(ctx, sec);
    return (1);
}

static void	handle_player_kv(armes_db *db, const char *key, const char *val)
{
    if (strcmp(key, "name") == 0)
        safe_copy(db->player_name, sizeof(db->player_name), val);
}

static void	handle_amp_kv(t_parse_ctx *ctx, const char *key, const char *val)
{
    if (strcmp(key, "amp_decay_shot") == 0)
        (void)parse_double_flex(val, &ctx->cur_amp.amp_decay_shot);
    else if (strcmp(key, "amp_mu") == 0)
    {
        (void)parse_double_flex(val, &ctx->cur_amp.amp_mu);
        if (ctx->cur_amp.amp_mu <= 0.0)
            ctx->cur_amp.amp_mu = 1.0;
    }
    else if (strcmp(key, "notes") == 0)
        safe_copy(ctx->cur_amp.notes, sizeof(ctx->cur_amp.notes), val);
}

static void	handle_arme_kv_mu(double *dst, const char *val, double def, int zero_ok)
{
    (void)parse_double_flex(val, dst);
    if (!zero_ok && *dst <= 0.0)
        *dst = def;
    if (zero_ok && *dst < 0.0)
        *dst = 0.0;
}

static void	handle_arme_kv(t_parse_ctx *ctx, const char *key, const char *val)
{
    if (strcmp(key, "dpp") == 0)
        (void)parse_double_flex(val, &ctx->cur.dpp);
    else if (strcmp(key, "ammo_shot") == 0)
        (void)parse_double_flex(val, &ctx->cur.ammo_shot);
    else if (strcmp(key, "decay_shot") == 0)
        (void)parse_double_flex(val, &ctx->cur.decay_shot);
    else if (strcmp(key, "amp_decay_shot") == 0)
        (void)parse_double_flex(val, &ctx->cur.amp_decay_shot);
    else if (strcmp(key, "markup") == 0)
        handle_arme_kv_mu(&ctx->cur.markup, val, 1.0, 0);
    else if (strcmp(key, "ammo_mu") == 0)
        handle_arme_kv_mu(&ctx->cur.ammo_mu, val, 1.0, 0);
    else if (strcmp(key, "weapon_mu") == 0)
        handle_arme_kv_mu(&ctx->cur.weapon_mu, val, 0.0, 1);
    else if (strcmp(key, "amp_mu") == 0)
        handle_arme_kv_mu(&ctx->cur.amp_mu, val, 0.0, 1);
    else if (strcmp(key, "notes") == 0)
        safe_copy(ctx->cur.notes, sizeof(ctx->cur.notes), val);
    else if (strcmp(key, "amp") == 0)
        safe_copy(ctx->cur.amp_name, sizeof(ctx->cur.amp_name), val);
}

static int	parse_kv_line(char *p, char **key, char **val)
{
    char	*eq;
    
    eq = strchr(p, '=');
    if (!eq)
        return (0);
    *eq = '\0';
    *key = str_ltrim(p);
    str_rtrim(*key);
    *val = str_ltrim(eq + 1);
    str_rtrim(*val);
    return (1);
}

static void	link_weapon_amps(armes_db *db)
{
    size_t			i;
    const amp_stats	*amp;
    
    i = 0;
    while (i < db->count)
    {
        if (db->items[i].amp_name[0] != '\0')
        {
            amp = find_amp(db, db->items[i].amp_name);
            if (amp)
            {
                db->items[i].amp_decay_shot = amp->amp_decay_shot;
                db->items[i].amp_mu = amp->amp_mu;
            }
            else
            {
                fprintf(stderr, "[WARN] Amp '%s' not found for '%s'\n",
                        db->items[i].amp_name, db->items[i].name);
            }
        }
        i++;
    }
}

static void	process_line(armes_db *db, t_parse_ctx *ctx, char *line)
{
    char	*p;
    char	*key;
    char	*val;
    
    str_rtrim(line);
    p = str_ltrim(line);
    if (line_is_comment_or_empty(p))
        return ;
    if (*p == '[')
        (void)handle_section_line(db, ctx, p);
    else if (ctx->have_section && parse_kv_line(p, &key, &val))
    {
        if (ctx->in_player)
            handle_player_kv(db, key, val);
        else if (ctx->in_amp)
            handle_amp_kv(ctx, key, val);
        else
            handle_arme_kv(ctx, key, val);
    }
}

int	armes_db_load(armes_db *db, const char *path)
{
    FILE		*fp;
    char		line[512];
    t_parse_ctx	ctx;
    
    if (!db || !path)
        return (0);
    db_init(db);
    fp = fopen(path, "r");
    if (!fp)
        return (0);
    ctx_init(&ctx);
    while (fgets(line, sizeof(line), fp))
        process_line(db, &ctx, line);
    save_previous_section(db, &ctx);
    link_weapon_amps(db);
    fclose(fp);
    return (1);
}


void	armes_db_free(armes_db *db)
{
    if (!db)
        return ;
    free(db->items);
    free(db->amps.items);
    db_init(db);
}

const arme_stats	*armes_db_find(const armes_db *db, const char *name)
{
    size_t	i;
    
    if (!db || !name)
        return (NULL);
    i = 0;
    while (i < db->count)
    {
        if (strcmp(db->items[i].name, name) == 0)
            return (&db->items[i]);
        i++;
    }
    return (NULL);
}

double	arme_cost_shot(const arme_stats *w)
{
    double	ammo_mu;
    double	weapon_mu;
    double	amp_mu;
    if (!w)
        return (0.0);
    if (w->weapon_mu > 0.0 || w->amp_mu > 0.0)
    {
        ammo_mu = 1.0;
        weapon_mu = 1.0;
        amp_mu = 1.0;
        if (w->ammo_mu > 0.0)
            ammo_mu = w->ammo_mu;
        if (w->weapon_mu > 0.0)
            weapon_mu = w->weapon_mu;
        if (w->amp_mu > 0.0)
            amp_mu = w->amp_mu;
        return (w->ammo_shot * ammo_mu
        + w->decay_shot * weapon_mu
        + w->amp_decay_shot * amp_mu);
    }
    return (w->ammo_shot
    + (w->decay_shot + w->amp_decay_shot) * w->markup);
}
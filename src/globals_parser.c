/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   globals_parser.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: entropia-tracker                              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25                                #+#    #+#             */
/*   Updated: 2026/01/25                                #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "globals_parser.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static void	safe_copy(char *dst, size_t dstsz, const char *src)
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

static int	is_ts_prefix(const char *s)
{
    /* "2026-01-21 10:42:49" => 19 chars */
    if (!s)
        return (0);
    for (int i = 0; i < 19; ++i)
    {
        if (!s[i])
            return (0);
    }
    return (isdigit((unsigned char)s[0]) && isdigit((unsigned char)s[1])
    && isdigit((unsigned char)s[2]) && isdigit((unsigned char)s[3])
    && s[4] == '-' && s[7] == '-' && s[10] == ' '
    && s[13] == ':' && s[16] == ':');
}

static void	extract_ts(const char *line, char *out, size_t outsz)
{
    if (is_ts_prefix(line))
    {
        char	tmp[32];
        memcpy(tmp, line, 19);
        tmp[19] = '\0';
        safe_copy(out, outsz, tmp);
    }
    else
        safe_copy(out, outsz, "");
}

static int	has_hof(const char *line)
{
    return (line && strstr(line, "A record has been added to the Hall of Fame!") != NULL);
}

static int	has_ath(const char *line)
{
    /* Dans ton chat.log fourni je ne vois pas encore de "ATH" explicite,
     * donc on supporte plusieurs signatures possibles. */
    if (!line)
        return (0);
    if (strstr(line, "ALL TIME HIGH") || strstr(line, "All Time High"))
        return (1);
    if (strstr(line, " ATH") || strstr(line, "(ATH") || strstr(line, "ATH!"))
        return (1);
    return (0);
}

static int	parse_between_parens(const char *p, char *out, size_t outsz)
{
    const char	*o;
    const char	*c;
    size_t		n;
    
    if (!p)
        return (0);
    o = strchr(p, '(');
    if (!o)
        return (0);
    c = strchr(o + 1, ')');
    if (!c)
        return (0);
    n = (size_t)(c - (o + 1));
    if (n == 0)
        return (0);
    if (n >= outsz)
        n = outsz - 1;
    memcpy(out, o + 1, n);
    out[n] = '\0';
    return (1);
}

static int	parse_value_number(const char *p, double *out)
{
    /* récupère un nombre juste après "value of " ou "worth " */
    char		buf[64];
    size_t		i = 0;
    const char	*s;
    char		*end;
    
    if (!p || !out)
        return (0);
    s = p;
    while (*s && isspace((unsigned char)*s))
        s++;
    while (s[i] && i + 1 < sizeof(buf))
    {
        if (!(isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == ','))
            break;
        buf[i] = (s[i] == ',') ? '.' : s[i];
        i++;
    }
    buf[i] = '\0';
    if (i == 0)
        return (0);
    *out = strtod(buf, &end);
    return (end != buf);
}

static void	set_type(char *dst, size_t dstsz, const char *base, int ath, int hof)
{
    /* priorité ATH > HOF > GLOB */
    if (ath)
    {
        char	tmp[64];
        snprintf(tmp, sizeof(tmp), "ATH_%s", base);
        safe_copy(dst, dstsz, tmp);
    }
    else if (hof)
    {
        char	tmp[64];
        snprintf(tmp, sizeof(tmp), "HOF_%s", base);
        safe_copy(dst, dstsz, tmp);
    }
    else
    {
        char	tmp[64];
        snprintf(tmp, sizeof(tmp), "GLOB_%s", base);
        safe_copy(dst, dstsz, tmp);
    }
}

int	globals_parse_line(const char *line, t_globals_event *ev)
{
    int		hof;
    int		ath;
    double	v;
    
    const char	*killed = "killed a creature (";
    const char	*crafted = "constructed an item (";
    const char	*rare = "has found a rare item (";
    
    const char	*p;
    
    if (!line || !ev)
        return (0);
    
    /* On ne prend QUE [Globals] */
    if (!strstr(line, "[Globals]"))
        return (0);
    
    memset(ev, 0, sizeof(*ev));
    extract_ts(line, ev->ts, sizeof(ev->ts));
    safe_copy(ev->raw, sizeof(ev->raw), line);
    
    hof = has_hof(line);
    ath = has_ath(line);
    
    /* 1) MOB: "... killed a creature (Mob) with a value of XX PED!" */
    p = strstr(line, killed);
    if (p)
    {
        if (!parse_between_parens(p, ev->name, sizeof(ev->name)))
            return (0);
        p = strstr(p, "with a value of ");
        if (!p)
            return (0);
        p += strlen("with a value of ");
        if (!parse_value_number(p, &v))
            return (0);
        snprintf(ev->value, sizeof(ev->value), "%.4f", v);
        set_type(ev->type, sizeof(ev->type), "MOB", ath, hof);
        return (1);
    }
    
    /* 2) CRAFT: "... constructed an item (Item) worth XX PED!" */
    p = strstr(line, crafted);
    if (p)
    {
        if (!parse_between_parens(p, ev->name, sizeof(ev->name)))
            return (0);
        p = strstr(p, "worth ");
        if (!p)
            return (0);
        p += strlen("worth ");
        if (!parse_value_number(p, &v))
            return (0);
        snprintf(ev->value, sizeof(ev->value), "%.4f", v);
        set_type(ev->type, sizeof(ev->type), "CRAFT", ath, hof);
        return (1);
    }
    
    /* 3) RARE ITEM: "... has found a rare item (X) with a value of 10 PEC!" */
    p = strstr(line, rare);
    if (p)
    {
        if (!parse_between_parens(p, ev->name, sizeof(ev->name)))
            return (0);
        p = strstr(p, "with a value of ");
        if (!p)
            return (0);
        p += strlen("with a value of ");
        if (!parse_value_number(p, &v))
            return (0);
        
        /* PEC -> PED si on voit "PEC" (cas exact dans ton log) */
        if (strstr(p, "PEC"))
            v = v / 100.0;
        
        snprintf(ev->value, sizeof(ev->value), "%.6f", v);
        set_type(ev->type, sizeof(ev->type), "RARE", ath, hof);
        return (1);
    }
    
    return (0);
}

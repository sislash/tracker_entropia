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

/* NOUVEAU : accepte EN + FR */
static int	is_globals_channel(const char *line)
{
    if (!line)
        return (0);
    if (strstr(line, "[Globals]"))
        return (1);
    if (strstr(line, "[Globaux]"))
        return (1);
    return (0);
}

/* NOUVEAU : HOF détecté via "Hall of Fame" (suffix, EN/FR) */
static int	has_hof(const char *line)
{
    if (!line)
        return (0);
    return (strstr(line, "Hall of Fame") != NULL);
}

static int	has_ath(const char *line)
{
    /* ATH pas toujours présent, on supporte large */
    if (!line)
        return (0);
    if (strstr(line, "ALL TIME HIGH") || strstr(line, "All Time High"))
        return (1);
    if (strstr(line, " ATH") || strstr(line, "(ATH") || strstr(line, "ATH!"))
        return (1);
    if (strstr(line, "RECORD ABSOLU") || strstr(line, "Record absolu"))
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
    /* récupère un nombre juste après "value of " / "worth " / "valeur de" */
    char		buf[64];
    size_t		i;
    const char	*s;
    char		*end;
    
    if (!p || !out)
        return (0);
    s = p;
    while (*s && isspace((unsigned char)*s))
        s++;
    i = 0;
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
    int			hof;
    int			ath;
    double		v;
    const char	*p;
    
    /* EN */
    const char	*killed_en = "killed a creature (";
    const char	*crafted_en = "constructed an item (";
    const char	*rare_en = "has found a rare item (";
    
    /* FR (formes les + fréquentes, robustes) */
    const char	*killed_fr = "a tue une creature (";           /* fallback sans accents */
    const char	*killed_fr2 = "a tué une créature (";          /* avec accents */
    const char	*crafted_fr = "a construit un objet (";        /* craft */
    const char	*crafted_fr2 = "a fabriqué un objet (";        /* autre variante */
    
    if (!line || !ev)
        return (0);
    
    /* NOUVEAU : On ne prend QUE [Globals] ou [Globaux] */
    if (!is_globals_channel(line))
        return (0);
    
    memset(ev, 0, sizeof(*ev));
    extract_ts(line, ev->ts, sizeof(ev->ts));
    safe_copy(ev->raw, sizeof(ev->raw), line);
    
    hof = has_hof(line);
    ath = has_ath(line);
    
    /* ========================================================= */
    /* 1) MOB EN : "... killed a creature (Mob) with a value of XX PED!" */
    p = strstr(line, killed_en);
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
    
    /* 1bis) MOB FR : "... a tué une créature (Mob) ... valeur de XX PED!" */
    p = strstr(line, killed_fr2);
    if (!p)
        p = strstr(line, killed_fr);
    if (p)
    {
        if (!parse_between_parens(p, ev->name, sizeof(ev->name)))
            return (0);
        
        /* plusieurs formulations possibles */
        if (strstr(p, "avec une valeur de "))
            p = strstr(p, "avec une valeur de ") + strlen("avec une valeur de ");
        else if (strstr(p, "d'une valeur de "))
            p = strstr(p, "d'une valeur de ") + strlen("d'une valeur de ");
        else if (strstr(p, "valeur de "))
            p = strstr(p, "valeur de ") + strlen("valeur de ");
        else
            return (0);
        
        if (!parse_value_number(p, &v))
            return (0);
        snprintf(ev->value, sizeof(ev->value), "%.4f", v);
        set_type(ev->type, sizeof(ev->type), "MOB", ath, hof);
        return (1);
    }
    
    /* ========================================================= */
    /* 2) CRAFT EN : "... constructed an item (Item) worth XX PED!" */
    p = strstr(line, crafted_en);
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
    
    /* 2bis) CRAFT FR : "... a construit/fabriqué un objet (Item) ... valeur de XX PED!" */
    p = strstr(line, crafted_fr);
    if (!p)
        p = strstr(line, crafted_fr2);
    if (p)
    {
        if (!parse_between_parens(p, ev->name, sizeof(ev->name)))
            return (0);
        
        if (strstr(p, "d'une valeur de "))
            p = strstr(p, "d'une valeur de ") + strlen("d'une valeur de ");
        else if (strstr(p, "avec une valeur de "))
            p = strstr(p, "avec une valeur de ") + strlen("avec une valeur de ");
        else if (strstr(p, "valant "))
            p = strstr(p, "valant ") + strlen("valant ");
        else if (strstr(p, "valeur de "))
            p = strstr(p, "valeur de ") + strlen("valeur de ");
        else
            return (0);
        
        if (!parse_value_number(p, &v))
            return (0);
        snprintf(ev->value, sizeof(ev->value), "%.4f", v);
        set_type(ev->type, sizeof(ev->type), "CRAFT", ath, hof);
        return (1);
    }
    
    /* ========================================================= */
    /* 3) RARE ITEM EN : "... has found a rare item (X) with a value of 10 PEC!" */
    p = strstr(line, rare_en);
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
        
        /* PEC -> PED si on voit "PEC" */
        if (strstr(p, "PEC"))
            v = v / 100.0;
        
        snprintf(ev->value, sizeof(ev->value), "%.6f", v);
        set_type(ev->type, sizeof(ev->type), "RARE", ath, hof);
        return (1);
    }
    
    return (0);
}

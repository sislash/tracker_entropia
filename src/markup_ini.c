/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   markup_ini.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: entropia-tracker                              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/27                                #+#    #+#             */
/*   Updated: 2026/01/27                                #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "markup_ini.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* from markup.c (internal insert/update) */
int	markup__db_insert_or_update(t_markup_db *db, const t_markup_rule *r);

static void	trim_left(char **s)
{
    while (**s && isspace((unsigned char)**s))
        (*s)++;
}

static void	trim_right(char *s)
{
    size_t	n;
    
    if (!s)
        return ;
    n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
    {
        s[n - 1] = '\0';
        n--;
    }
}

static int	is_empty_or_comment(const char *s)
{
    if (!s)
        return (1);
    while (*s && isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return (1);
    if (*s == ';' || *s == '#')
        return (1);
    return (0);
}

static int	parse_section(const char *line, char *out, size_t outsz)
{
    const char	*o;
    const char	*c;
    size_t		n;
    
    o = strchr(line, '[');
    if (!o)
        return (0);
    c = strchr(o + 1, ']');
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

static int	streq_nocase(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return (0);
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

static int	parse_key_value(char *line, char *key, size_t ksz, char *val, size_t vsz)
{
    char	*eq;
    char	*s;
    
    key[0] = '\0';
    val[0] = '\0';
    
    s = line;
    trim_left(&s);
    trim_right(s);
    if (is_empty_or_comment(s))
        return (0);
    
    eq = strchr(s, '=');
    if (!eq)
        return (0);
    
    *eq = '\0';
    
    /* key */
    trim_right(s);
    while (*s && isspace((unsigned char)*s))
        s++;
    strncpy(key, s, ksz - 1);
    key[ksz - 1] = '\0';
    
    /* val */
    eq++;
    trim_left(&eq);
    trim_right(eq);
    strncpy(val, eq, vsz - 1);
    val[vsz - 1] = '\0';
    return (1);
}

static int	parse_type_value(const char *s, t_markup_type *out)
{
    if (!s || !out)
        return (0);
    if (streq_nocase(s, "percent"))
    {
        *out = MARKUP_PERCENT;
        return (1);
    }
    if (streq_nocase(s, "tt_plus"))
    {
        *out = MARKUP_TT_PLUS;
        return (1);
    }
    return (0);
}

int	markup_ini_parse_file(t_markup_db *db, const char *path)
{
    FILE			*f;
    char			buf[512];
    
    t_markup_rule	cur;
    int				in_section;
    int				has_type;
    int				has_value;
    
    char			section[128];
    char			key[64];
    char			val[256];
    
    if (!db || !path)
        return (-1);
    f = fopen(path, "rb");
    if (!f)
        return (-1);
    
    memset(&cur, 0, sizeof(cur));
    in_section = 0;
    has_type = 0;
    has_value = 0;
    
    while (fgets(buf, sizeof(buf), f))
    {
        char *line = buf;
        
        trim_left(&line);
        trim_right(line);
        if (is_empty_or_comment(line))
            continue;
        
        if (line[0] == '[')
        {
            /* flush previous */
            if (in_section && cur.name[0] && has_value)
            {
                if (!has_type)
                    cur.type = MARKUP_PERCENT;
                if (markup__db_insert_or_update(db, &cur) != 0)
                {
                    fclose(f);
                    return (-1);
                }
            }
            /* start new section */
            memset(&cur, 0, sizeof(cur));
            has_type = 0;
            has_value = 0;
            
            if (!parse_section(line, section, sizeof(section)))
                continue;
            strncpy(cur.name, section, sizeof(cur.name) - 1);
            cur.name[sizeof(cur.name) - 1] = '\0';
            in_section = 1;
            continue;
        }
        
        if (!in_section)
            continue;
        
        if (parse_key_value(line, key, sizeof(key), val, sizeof(val)) == 0)
            continue;
        
        if (streq_nocase(key, "type"))
        {
            if (parse_type_value(val, &cur.type))
                has_type = 1;
        }
        else if (streq_nocase(key, "value"))
        {
            cur.value = strtod(val, NULL);
            has_value = 1;
        }
    }
    
    /* flush last */
    if (in_section && cur.name[0] && has_value)
    {
        if (!has_type)
            cur.type = MARKUP_PERCENT;
        if (markup__db_insert_or_update(db, &cur) != 0)
        {
            fclose(f);
            return (-1);
        }
    }
    
    fclose(f);
    return (0);
}

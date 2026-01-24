#define _CRT_SECURE_NO_WARNINGS

#include "config_arme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void rtrim(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || isspace((unsigned char)s[n - 1])))
        s[--n] = '\0';
}

static char *ltrim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static int is_comment_or_empty(const char *s)
{
    if (!s) return 1;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return 1;
    return (*s == ';' || *s == '#');
}

static int parse_double_flex(const char *s, double *out)
{
    if (!s) return 0;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return 0;
    
    char buf[128];
    size_t j = 0;
    for (size_t i = 0; s[i] && j + 1 < sizeof(buf); ++i)
    {
        char ch = s[i];
        if (ch == ',') ch = '.';
        buf[j++] = ch;
    }
    buf[j] = '\0';
    
    char *end = NULL;
    double v = strtod(buf, &end);
    if (end == buf) return 0;
    *out = v;
    return 1;
}

static void arme_defaults(arme_stats *w)
{
    memset(w, 0, sizeof(*w));
    w->markup = 1.0;
}

static int db_push(armes_db *db, const arme_stats *w)
{
    arme_stats *tmp = (arme_stats*)realloc(db->items, (db->count + 1) * sizeof(arme_stats));
    if (!tmp) return 0;
    db->items = tmp;
    db->items[db->count] = *w;
    db->count++;
    return 1;
}

int armes_db_load(armes_db *db, const char *path)
{
    if (!db || !path) return 0;
    
    db->items = NULL;
    db->count = 0;
    db->player_name[0] = '\0';
    
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    
    char line[512];
    
    arme_stats current;
    int have_section = 0;
    int in_player_section = 0;
    
    arme_defaults(&current);
    
    while (fgets(line, sizeof(line), fp))
    {
        rtrim(line);
        char *p = ltrim(line);
        
        if (is_comment_or_empty(p))
            continue;
        
        /* Section [Nom] */
        if (*p == '[')
        {
            char *end = strchr(p, ']');
            if (!end) continue;
            *end = '\0';
            
            /* Sauvegarde de la section arme précédente */
            if (have_section && !in_player_section)
                (void)db_push(db, &current);
            
            /* Nouvelle section */
            have_section = 1;
            in_player_section = 0;
            
            if (strcmp(p + 1, "PLAYER") == 0)
            {
                in_player_section = 1;
            }
            else
            {
                arme_defaults(&current);
                snprintf(current.name, sizeof(current.name), "%s", p + 1);
            }
            continue;
        }
        
        /* key=value */
        char *eq = strchr(p, '=');
        if (!eq || !have_section) continue;
        
        *eq = '\0';
        char *key = ltrim(p);
        rtrim(key);
        
        char *val = ltrim(eq + 1);
        rtrim(val);
        
        if (in_player_section)
        {
            if (strcmp(key, "name") == 0)
                snprintf(db->player_name, sizeof(db->player_name), "%s", val);
            continue;
        }
        
        /* Section arme */
        if (strcmp(key, "dpp") == 0) (void)parse_double_flex(val, &current.dpp);
        else if (strcmp(key, "ammo_shot") == 0) (void)parse_double_flex(val, &current.ammo_shot);
        else if (strcmp(key, "decay_shot") == 0) (void)parse_double_flex(val, &current.decay_shot);
        else if (strcmp(key, "amp_decay_shot") == 0) (void)parse_double_flex(val, &current.amp_decay_shot);
        else if (strcmp(key, "markup") == 0)
        {
            (void)parse_double_flex(val, &current.markup);
            if (current.markup <= 0.0) current.markup = 1.0;
        }
        else if (strcmp(key, "notes") == 0)
        {
            snprintf(current.notes, sizeof(current.notes), "%s", val);
        }
    }
    
    /* push dernière section arme si besoin */
    if (have_section && !in_player_section)
        (void)db_push(db, &current);
    
    fclose(fp);
    return 1;
}

void armes_db_free(armes_db *db)
{
    if (!db) return;
    free(db->items);
    db->items = NULL;
    db->count = 0;
    db->player_name[0] = '\0';
}

const arme_stats *armes_db_find(const armes_db *db, const char *name)
{
    if (!db || !name) return NULL;
    for (size_t i = 0; i < db->count; ++i)
        if (strcmp(db->items[i].name, name) == 0)
            return &db->items[i];
    return NULL;
}

double arme_cost_shot(const arme_stats *w)
{
    if (!w) return 0.0;
    return (w->ammo_shot + w->decay_shot + w->amp_decay_shot) * w->markup;
}

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
    
    /* NEW (EU) */
    w->ammo_mu = 1.0;
    w->weapon_mu = 0.0; /* 0.0 = non défini */
    w->amp_mu = 0.0;    /* 0.0 = non défini */
    
    /* RÈGLE AMP : vide = pas d'amp */
    w->amp_name[0] = '\0';
}

/* ============================
 *  AJOUTS (AMP DB)
 * ============================ */

static void amp_defaults(amp_stats *a)
{
    memset(a, 0, sizeof(*a));
    a->amp_mu = 1.0;
}

static int db_amp_push(armes_db *db, const amp_stats *a)
{
    amp_stats *tmp;
    
    tmp = (amp_stats*)realloc(db->amps.items, (db->amps.count + 1) * sizeof(amp_stats));
    if (!tmp)
        return 0;
    db->amps.items = tmp;
    db->amps.items[db->amps.count] = *a;
    db->amps.count++;
    return 1;
}

static const amp_stats *find_amp(const armes_db *db, const char *name)
{
    size_t i;
    
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
    
    /* IMPORTANT: init AMP DB */
    db->amps.items = NULL;
    db->amps.count = 0;
    
    db->player_name[0] = '\0';
    
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    
    char line[512];
    
    arme_stats current;
    amp_stats  current_amp;
    
    int have_section = 0;
    int in_player_section = 0;
    int in_amp_section = 0;
    
    arme_defaults(&current);
    amp_defaults(&current_amp);
    
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
            
            /* Sauvegarde de la section précédente (arme/amp) */
            if (have_section)
            {
                if (in_amp_section)
                    (void)db_amp_push(db, &current_amp);
                else if (!in_player_section)
                    (void)db_push(db, &current);
            }
            
            /* Nouvelle section */
            have_section = 1;
            in_player_section = 0;
            in_amp_section = 0;
            
            /* Nom de section */
            const char *sec = p + 1;
            
            if (strcmp(sec, "PLAYER") == 0)
            {
                in_player_section = 1;
            }
            else if (strncmp(sec, "AMP:", 4) == 0)
            {
                in_amp_section = 1;
                amp_defaults(&current_amp);
                snprintf(current_amp.name, sizeof(current_amp.name), "%s", sec + 4);
            }
            else
            {
                arme_defaults(&current);
                snprintf(current.name, sizeof(current.name), "%s", sec);
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
        
        /* Section AMP */
        if (in_amp_section)
        {
            if (strcmp(key, "amp_decay_shot") == 0)
                (void)parse_double_flex(val, &current_amp.amp_decay_shot);
            else if (strcmp(key, "amp_mu") == 0)
            {
                (void)parse_double_flex(val, &current_amp.amp_mu);
                if (current_amp.amp_mu <= 0.0) current_amp.amp_mu = 1.0;
            }
            else if (strcmp(key, "notes") == 0)
                snprintf(current_amp.notes, sizeof(current_amp.notes), "%s", val);
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
        else if (strcmp(key, "ammo_mu") == 0)
        {
            (void)parse_double_flex(val, &current.ammo_mu);
            if (current.ammo_mu <= 0.0) current.ammo_mu = 1.0;
        }
        else if (strcmp(key, "weapon_mu") == 0)
        {
            (void)parse_double_flex(val, &current.weapon_mu);
            if (current.weapon_mu <= 0.0) current.weapon_mu = 0.0;
        }
        else if (strcmp(key, "amp_mu") == 0)
        {
            (void)parse_double_flex(val, &current.amp_mu);
            if (current.amp_mu <= 0.0) current.amp_mu = 0.0;
        }
        else if (strcmp(key, "notes") == 0)
        {
            snprintf(current.notes, sizeof(current.notes), "%s", val);
        }
        else if (strcmp(key, "amp") == 0)
        {
            strncpy(current.amp_name, val, sizeof(current.amp_name) - 1);
            current.amp_name[sizeof(current.amp_name) - 1] = '\0';
        }
    }
    
    /* push dernière section (arme ou amp) si besoin */
    if (have_section)
    {
        if (in_amp_section)
            (void)db_amp_push(db, &current_amp);
        else if (!in_player_section)
            (void)db_push(db, &current);
    }
    
    /* LIAISON: arme -> amp (APRÈS avoir tout push) */
    size_t i;
    const amp_stats *amp;
    
    i = 0;
    while (i < db->count)
    {
        /* RÈGLE: si amp_name vide → pas d'amp */
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
                fprintf(stderr,
                        "[WARN] Amp '%s' introuvable pour arme '%s'\n",
                        db->items[i].amp_name,
                        db->items[i].name);
            }
        }
        i++;
    }
    
    fclose(fp);
    return 1;
}

void armes_db_free(armes_db *db)
{
    if (!db) return;
    free(db->items);
    free(db->amps.items);
    db->items = NULL;
    db->count = 0;
    
    db->amps.items = NULL;
    db->amps.count = 0;
    
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

double	arme_cost_shot(const arme_stats *w)
{
    double	ammo_mu;
    double	weapon_mu;
    double	amp_mu;
    
    if (!w)
        return (0.0);
    if (w->weapon_mu > 0.0 || w->amp_mu > 0.0)
    {
        ammo_mu = (w->ammo_mu > 0.0) ? w->ammo_mu : 1.0;
        weapon_mu = (w->weapon_mu > 0.0) ? w->weapon_mu : 1.0;
        amp_mu = (w->amp_mu > 0.0) ? w->amp_mu : 1.0;
        return (w->ammo_shot * ammo_mu
        + w->decay_shot * weapon_mu
        + w->amp_decay_shot * amp_mu);
    }
    return (w->ammo_shot
    + (w->decay_shot + w->amp_decay_shot) * w->markup);
}

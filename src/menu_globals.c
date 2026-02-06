/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   menu_globals.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/02/05 00:00:00 by you              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "menu_globals.h"
#include "menu_principale.h"

#include "window.h"
#include "utils.h"

#include "core_paths.h"
#include "globals_stats.h"
#include "globals_thread.h"
#include "csv.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ************************************************************************** */
/*  Helpers                                                                   */
/* ************************************************************************** */

static void	w_draw_lines(t_window *w, int x, int y, const char **lines, int n)
{
    int	i;
    int	line_h;
    
    if (!w || !lines || n <= 0)
        return ;
    line_h = 18;
    i = 0;
    while (i < n)
    {
        if (lines[i] && lines[i][0])
            window_draw_text(w, x, y + i * line_h, lines[i], 0x000000);
        i++;
    }
}

static void	fmt_linef(char *dst, size_t cap, const char *k, const char *fmt, ...)
{
    va_list	ap;
    char	buf[256];
    
    if (!dst || cap == 0)
        return ;
    buf[0] = '\0';
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    snprintf(dst, cap, "%s: %s", k, buf);
}

/* ************************************************************************** */
/*  CSV helpers                                                               */
/* ************************************************************************** */

static void	ensure_globals_csv(void)
{
    FILE	*f;
    
    f = fopen(tm_path_globals_csv(), "ab");
    if (!f)
        return ;
    csv_ensure_header6(f);
    fclose(f);
}

/* ************************************************************************** */
/*  Generic message screen                                                    */
/* ************************************************************************** */

static void	screen_message(t_window *w, const char *title,
                           const char **lines, int n)
{
    const char	*items[1];
    t_menu		m;
    int			action;
    
    items[0] = "Retour";
    menu_init(&m, items, 1);
    while (w->running)
    {
        window_poll_events(w);
        if (w->key_escape)
            break ;
        action = menu_update(&m, w);
        if (action == 0)
            break ;
        window_clear(w, 0xFFFFFF);
        if (title)
            window_draw_text(w, 30, 30, title, 0x000000);
        if (lines && n > 0)
            w_draw_lines(w, 30, 70, lines, n);
        menu_render(&m, w, 30, 520);
        window_present(w);
        ft_sleep_ms(16);
    }
}

/* ************************************************************************** */
/*  Confirm screen (fenetre)                                                  */
/* ************************************************************************** */

static int	screen_confirm_yes_menu(t_window *w, const char *title,
                                    const char **lines, int n)
{
    const char	*items[2];
    t_menu		m;
    int			action;
    
    /* On garde le wording "YES", mais on le fait via menu (pas de saisie texte). */
    items[0] = "Confirmer (YES)";
    items[1] = "Annuler";
    menu_init(&m, items, 2);
    while (w->running)
    {
        window_poll_events(w);
        if (w->key_escape)
            return (0);
        action = menu_update(&m, w);
        if (action == 0)
            return (1);
        if (action == 1)
            return (0);
        
        window_clear(w, 0xFFFFFF);
        if (title)
            window_draw_text(w, 30, 30, title, 0x000000);
        if (lines && n > 0)
            w_draw_lines(w, 30, 70, lines, n);
        window_draw_text(w, 30, 440, "Selectionne 'Confirmer (YES)' pour vider le CSV.", 0x000000);
        menu_render(&m, w, 30, 500);
        window_present(w);
        ft_sleep_ms(16);
    }
    return (0);
}

/* ************************************************************************** */
/*  Actions                                                                   */
/* ************************************************************************** */

static void	action_clear_globals_csv(t_window *w)
{
    FILE		*f;
    const char	*info[3];
    const char	*ok[1];
    const char	*err[1];
    
    info[0] = "ATTENTION : tu vas vider le fichier :";
    info[1] = tm_path_globals_csv();
    info[2] = "";
    
    if (!screen_confirm_yes_menu(w, "VIDER CSV GLOBALS", info, 3))
    {
        const char *msg[] = { "Annule." };
        screen_message(w, "VIDER CSV GLOBALS", msg, 1);
        return ;
    }
    f = fopen(tm_path_globals_csv(), "w");
    if (!f)
    {
        err[0] = "[ERREUR] Impossible d'ouvrir le CSV en ecriture.";
        screen_message(w, "VIDER CSV GLOBALS", err, 1);
        return ;
    }
    fclose(f);
    ensure_globals_csv();
    ok[0] = "OK : CSV globals vide.";
    screen_message(w, "VIDER CSV GLOBALS", ok, 1);
}

/* ************************************************************************** */
/*  Dashboard helpers                                                         */
/* ************************************************************************** */

#define GDASH_BAR_MAX 26

static int	gdash_bar_fill(double v, double vmax)
{
    int	nb;
    
    if (vmax <= 0.0)
        return (0);
    nb = (int)((v / vmax) * (double)GDASH_BAR_MAX);
    if (nb < 0)
        nb = 0;
    if (nb > GDASH_BAR_MAX)
        nb = GDASH_BAR_MAX;
    return (nb);
}

static void	gdash_bar_str(char out[GDASH_BAR_MAX + 1], double v, double vmax)
{
    int	j;
    int	nb;
    
    nb = gdash_bar_fill(v, vmax);
    j = 0;
    while (j < GDASH_BAR_MAX)
    {
        out[j] = (j < nb) ? '#' : ' ';
        j++;
    }
    out[GDASH_BAR_MAX] = '\0';
}

static double	gdash_max_sum(const t_globals_top *arr, size_t cnt)
{
    double	m;
    size_t	i;
    
    m = 0.0;
    i = 0;
    while (i < cnt)
    {
        if (arr[i].sum_ped > m)
            m = arr[i].sum_ped;
        i++;
    }
    return (m);
}

static void	screen_dashboard_globals_live(t_window *w)
{
    t_globals_stats	s;
    /*
     * * Window dashboard: afficher aussi les tops avec (nom, barre, count).
     ** (équivalent de globals_view.c en console)
     */
    char			buf[96][256];
    const char		*lines[96];
    int				n;
    int				i;
    size_t			k;
    double			mx;
    
    while (w->running)
    {
        window_poll_events(w);
        if (w->key_escape)
            break ;
        
        memset(&s, 0, sizeof(s));
        (void)globals_stats_compute(tm_path_globals_csv(), 0, &s);
        
        n = 0;
        snprintf(buf[n++], sizeof(buf[0]), "DASHBOARD GLOBALS LIVE (fenetre)");
        fmt_linef(buf[n++], sizeof(buf[0]), "CSV", "%s", tm_path_globals_csv());
        buf[n++][0] = '\0';
        
        fmt_linef(buf[n++], sizeof(buf[0]), "Lignes lues", "%ld", s.data_lines_read);
        fmt_linef(buf[n++], sizeof(buf[0]), "Mob events", "%ld (%.4f PED)", s.mob_events, s.mob_sum_ped);
        fmt_linef(buf[n++], sizeof(buf[0]), "Craft events", "%ld (%.4f PED)", s.craft_events, s.craft_sum_ped);
        if (s.rare_events > 0)
            fmt_linef(buf[n++], sizeof(buf[0]), "Rare events", "%ld (%.4f PED)", s.rare_events, s.rare_sum_ped);
        buf[n++][0] = '\0';
        
        /* TOP MOBS */
        snprintf(buf[n++], sizeof(buf[0]), "TOP MOBS (somme PED)");
        snprintf(buf[n++], sizeof(buf[0]), "  #  %-24s %10s |%*s| %6s",
                 "Nom", "PED", GDASH_BAR_MAX, "", "Count");
        mx = gdash_max_sum(s.top_mobs, s.top_mobs_count);
        if (s.top_mobs_count == 0)
            snprintf(buf[n++], sizeof(buf[0]), "  (aucun)");
        k = 0;
        while (k < s.top_mobs_count && k < 10 && n < 94)
        {
            char bar[GDASH_BAR_MAX + 1];
            gdash_bar_str(bar, s.top_mobs[k].sum_ped, mx);
            snprintf(buf[n++], sizeof(buf[0]),
                     "%3zu) %-24.24s %10.2f |%s| %6ld",
                     k + 1, s.top_mobs[k].name, s.top_mobs[k].sum_ped,
                     bar, s.top_mobs[k].count);
            k++;
        }
        buf[n++][0] = '\0';
        
        /* TOP CRAFTS */
        snprintf(buf[n++], sizeof(buf[0]), "TOP CRAFTS (somme PED)");
        snprintf(buf[n++], sizeof(buf[0]), "  #  %-24s %10s |%*s| %6s",
                 "Nom", "PED", GDASH_BAR_MAX, "", "Count");
        mx = gdash_max_sum(s.top_crafts, s.top_crafts_count);
        if (s.top_crafts_count == 0)
            snprintf(buf[n++], sizeof(buf[0]), "  (aucun)");
        k = 0;
        while (k < s.top_crafts_count && k < 10 && n < 94)
        {
            char bar[GDASH_BAR_MAX + 1];
            gdash_bar_str(bar, s.top_crafts[k].sum_ped, mx);
            snprintf(buf[n++], sizeof(buf[0]),
                     "%3zu) %-24.24s %10.2f |%s| %6ld",
                     k + 1, s.top_crafts[k].name, s.top_crafts[k].sum_ped,
                     bar, s.top_crafts[k].count);
            k++;
        }
        buf[n++][0] = '\0';
        
        /* TOP RARES (si dispo) */
        if (s.rare_events > 0)
        {
            snprintf(buf[n++], sizeof(buf[0]), "TOP RARE ITEMS (somme PED)");
            snprintf(buf[n++], sizeof(buf[0]), "  #  %-24s %10s |%*s| %6s",
                     "Nom", "PED", GDASH_BAR_MAX, "", "Count");
            mx = gdash_max_sum(s.top_rares, s.top_rares_count);
            if (s.top_rares_count == 0)
                snprintf(buf[n++], sizeof(buf[0]), "  (aucun)");
            k = 0;
            while (k < s.top_rares_count && k < 10 && n < 94)
            {
                char bar[GDASH_BAR_MAX + 1];
                gdash_bar_str(bar, s.top_rares[k].sum_ped, mx);
                snprintf(buf[n++], sizeof(buf[0]),
                         "%3zu) %-24.24s %10.2f |%s| %6ld",
                         k + 1, s.top_rares[k].name, s.top_rares[k].sum_ped,
                         bar, s.top_rares[k].count);
                k++;
            }
            buf[n++][0] = '\0';
        }
        
        snprintf(buf[n++], sizeof(buf[0]), "(Auto-refresh ~250ms | Echap pour quitter)");
        
        i = 0;
        while (i < n)
        {
            lines[i] = buf[i];
            i++;
        }
        window_clear(w, 0xFFFFFF);
        w_draw_lines(w, 30, 30, lines, n);
        window_present(w);
        ft_sleep_ms(250);
    }
}

/* ************************************************************************** */
/*  Entry                                                                     */
/* ************************************************************************** */

void	menu_globals(t_window *w)
{
    t_menu		menu;
    int			action;
    const char	*items[6];
    uint64_t	frame_start;
    uint64_t	frame_ms;
    int			sleep_ms;
    int         done;
    
    done = 0;
    
    if (tm_ensure_logs_dir() != 0)
    {
        /* pas bloquant */
    }
    ensure_globals_csv();
    
    items[0] = "Demarrer LIVE (globals)";
    items[1] = "Demarrer REPLAY (globals)";
    items[2] = "Arreter le parser globals";
    items[3] = "Dashboard LIVE";
    items[4] = "Vider CSV globals (confirmation YES)";
    items[5] = "Retour";
    
    menu_init(&menu, items, 6);
    /* Utilise la fenetre existante (mode fenetre unique) */
    while (w && w->running && !done)
    {
        frame_start = ft_time_ms();
        window_poll_events(w);
        
        action = menu_update(&menu, w);
        if (action >= 0)
        {
            if (action == 0)
                globals_thread_start_live();
            else if (action == 1)
                globals_thread_start_replay();
            else if (action == 2)
            {
                globals_thread_stop();
                {
                    const char *msg[] = { "OK : parser GLOBALS arrete." };
                    screen_message(w, "PARSER GLOBALS", msg, 1);
                }
            }
            else if (action == 3)
                screen_dashboard_globals_live(w);
            else if (action == 4)
                action_clear_globals_csv(w);
            else
                done = 1;
        }
        if (w->key_escape)
            done = 1;
        
        window_clear(w, 0xFFFFFF);
        window_draw_text(w, 30, 20, "MENU GLOBALS / GLOBAUX (mode fenetre)", 0x000000);
        window_draw_text(w, 30, 45, "Fleches: selection | Entree: valider | Echap: quitter", 0x000000);
        {
            char st[128];
            snprintf(st, sizeof(st), "Etat parser globals : %s",
                     globals_thread_is_running() ? "EN COURS" : "ARRETE");
            window_draw_text(w, 30, 65, st, 0x000000);
        }
        menu_render(&menu, w, 30, 90);
        window_present(w);
        
        frame_ms = ft_time_ms() - frame_start;
        sleep_ms = 16 - (int)frame_ms;
        if (sleep_ms > 0)
            ft_sleep_ms(sleep_ms);
    }
    /* Reset ESC pour ne pas quitter le menu principal immediatement */
    if (w)
        w->key_escape = 0;
}

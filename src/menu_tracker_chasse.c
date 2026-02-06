/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   menu_tracker_chasse.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/02/05 00:00:00 by you              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "menu_tracker_chasse.h"

#include "menu_principale.h" /* t_menu + menu_update/menu_render_screen */
#include "window.h"
#include "utils.h"

#include "core_paths.h"
#include "config_arme.h"
#include "parser_thread.h"
#include "session.h"
#include "tracker_stats.h"
#include "weapon_selected.h"
#include "sweat_option.h"
#include "session_export.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* ************************************************************************** */
/*  Small helpers (window text)                                               */
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
		if (lines[i])
			window_draw_text(w, x, y + i * line_h, lines[i], 0x000000);
		i++;
	}
}

static long	ped_to_pec(double ped)
{
	return ((long)llround(ped * 100.0));
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
/*  Weapons helpers                                                           */
/* ************************************************************************** */

static int	load_db(armes_db *db)
{
	if (!db)
		return (-1);
	memset(db, 0, sizeof(*db));
	if (!armes_db_load(db, tm_path_armes_ini()))
		return (-1);
	return (0);
}

static int	load_selected_weapon(char *selected, size_t size)
{
	if (!selected || size == 0)
		return (0);
	selected[0] = '\0';
	if (weapon_selected_load(tm_path_weapon_selected(), selected, size) != 0)
		return (0);
	return (selected[0] != '\0');
}

static void	weapon_build_lines(const arme_stats *w, char out[16][256], int *n)
{
	int	k;
	
	if (!n)
		return ;
	*n = 0;
	if (!w)
		return ;
	k = 0;
	snprintf(out[k++], sizeof(out[0]), "ARME ACTIVE");
	fmt_linef(out[k++], sizeof(out[0]), "Nom", "%s", w->name);
	fmt_linef(out[k++], sizeof(out[0]), "DPP", "%.4f", w->dpp);
	fmt_linef(out[k++], sizeof(out[0]), "Ammo / tir", "%.6f PED", w->ammo_shot);
	fmt_linef(out[k++], sizeof(out[0]), "Decay / tir", "%.6f PED", w->decay_shot);
	fmt_linef(out[k++], sizeof(out[0]), "Amp decay / tir", "%.6f PED", w->amp_decay_shot);
	fmt_linef(out[k++], sizeof(out[0]), "Markup", "%.4f", w->markup);
	fmt_linef(out[k++], sizeof(out[0]), "Cout / tir total", "%.6f PED", arme_cost_shot(w));
	if (w->notes[0])
		fmt_linef(out[k++], sizeof(out[0]), "Notes", "%s", w->notes);
	*n = k;
}

/* ************************************************************************** */
/*  Hunt stats pages (simple version fenetre)                                 */
/* ************************************************************************** */

typedef enum e_dash_page
{
	DASH_RESUME = 0,
	DASH_LOOT,
	DASH_EXPENSES,
	DASH_RESULTS,
	DASH_COUNT
}	t_dash_page;

static double	safe_div(double a, double b)
{
	if (b == 0.0)
		return (0.0);
	return (a / b);
}

static double	ratio_pct(double num, double den)
{
	if (den <= 0.0)
		return (0.0);
	return ((num / den) * 100.0);
}

static void	fill_common_header(char lines[32][256], int *n, long offset)
{
	int	k;
	
	k = 0;
	snprintf(lines[k++], sizeof(lines[0]), "=== DASHBOARD CHASSE (fenetre) ===");
	fmt_linef(lines[k++], sizeof(lines[0]), "Parser",
			  "%s", parser_thread_is_running() ? "EN COURS" : "ARRETE");
	fmt_linef(lines[k++], sizeof(lines[0]), "Offset session", "%ld ligne(s)", offset);
	fmt_linef(lines[k++], sizeof(lines[0]), "CSV", "%s", tm_path_hunt_csv());
	lines[k++][0] = '\0';
	*n = k;
}

static void	dash_page_resume(const t_hunt_stats *s, char lines[32][256], int *n)
{
	int	k;
	
	k = *n;
	snprintf(lines[k++], sizeof(lines[0]), "[PAGE 1/%d] RESUME  (Q/D changer, Echap quitter)", DASH_COUNT);
	fmt_linef(lines[k++], sizeof(lines[0]), "Kills / Shots", "%ld / %ld", s->kills, s->shots);
	fmt_linef(lines[k++], sizeof(lines[0]), "Loot total", "%.4f PED (%ld PEC)", s->loot_ped, ped_to_pec(s->loot_ped));
	fmt_linef(lines[k++], sizeof(lines[0]), "Depense utilisee", "%.4f PED (%ld PEC)", s->expense_used, ped_to_pec(s->expense_used));
	fmt_linef(lines[k++], sizeof(lines[0]), "Net (profit/perte)", "%.4f PED (%ld PEC)", s->net_ped, ped_to_pec(s->net_ped));
	fmt_linef(lines[k++], sizeof(lines[0]), "Return", "%.2f %%", ratio_pct(s->loot_ped, s->expense_used));
	fmt_linef(lines[k++], sizeof(lines[0]), "Profit", "%.2f %%", ratio_pct(s->net_ped, s->expense_used));
	fmt_linef(lines[k++], sizeof(lines[0]), "Shots / kill", "%.2f", safe_div((double)s->shots, (double)s->kills));
	*n = k;
}

static void	dash_page_loot(const t_hunt_stats *s, char lines[32][256], int *n)
{
	int	k;
	
	k = *n;
	snprintf(lines[k++], sizeof(lines[0]), "[PAGE 2/%d] LOOT  (Q/D changer, Echap quitter)", DASH_COUNT);
	fmt_linef(lines[k++], sizeof(lines[0]), "Loot / shot", "%.6f", safe_div(s->loot_ped, (double)s->shots));
	fmt_linef(lines[k++], sizeof(lines[0]), "Loot / kill", "%.4f", safe_div(s->loot_ped, (double)s->kills));
	fmt_linef(lines[k++], sizeof(lines[0]), "Loot events", "%ld", s->loot_events);
	fmt_linef(lines[k++], sizeof(lines[0]), "Sweat events", "%ld", s->sweat_events);
	fmt_linef(lines[k++], sizeof(lines[0]), "Sweat total", "%ld", s->sweat_total);
	*n = k;
}

static void	dash_page_expenses(const t_hunt_stats *s, char lines[32][256], int *n)
{
	int	k;
	
	k = *n;
	snprintf(lines[k++], sizeof(lines[0]), "[PAGE 3/%d] DEPENSES  (Q/D changer, Echap quitter)", DASH_COUNT);
	fmt_linef(lines[k++], sizeof(lines[0]), "Depenses (CSV)", "%.4f PED", s->expense_ped_logged);
	fmt_linef(lines[k++], sizeof(lines[0]), "Depenses (modele)", "%.4f PED", s->expense_ped_calc);
	fmt_linef(lines[k++], sizeof(lines[0]), "Cout / shot", "%.6f PED", s->cost_shot);
	fmt_linef(lines[k++], sizeof(lines[0]), "Source depense", "%s", s->expense_used_is_logged ? "log CSV" : "modele arme");
	*n = k;
}

static void	dash_page_results(const t_hunt_stats *s, char lines[32][256], int *n)
{
	int	k;
	
	k = *n;
	snprintf(lines[k++], sizeof(lines[0]), "[PAGE 4/%d] RESULTATS  (Q/D changer, Echap quitter)", DASH_COUNT);
	fmt_linef(lines[k++], sizeof(lines[0]), "Net / shot", "%.6f", safe_div(s->net_ped, (double)s->shots));
	fmt_linef(lines[k++], sizeof(lines[0]), "Net / kill", "%.4f", safe_div(s->net_ped, (double)s->kills));
	fmt_linef(lines[k++], sizeof(lines[0]), "Loot/Expense events", "%ld / %ld", s->loot_events, s->expense_events);
	*n = k;
}

static void	dash_build_lines(const t_hunt_stats *s, long offset, t_dash_page page,
							 char lines[32][256], int *n)
{
	fill_common_header(lines, n, offset);
	if (page == DASH_RESUME)
		dash_page_resume(s, lines, n);
	else if (page == DASH_LOOT)
		dash_page_loot(s, lines, n);
	else if (page == DASH_EXPENSES)
		dash_page_expenses(s, lines, n);
	else
		dash_page_results(s, lines, n);
}

/* ************************************************************************** */
/*  Window screens                                                            */
/* ************************************************************************** */

static void	screen_message(t_window *w, const char *title, const char **lines, int n)
{
	int		action;
	const char	*items[1];
	t_menu		m;
	
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

static void	screen_weapon_active(t_window *w)
{
	armes_db				db;
	char					selected[128];
	const arme_stats		*wstat;
	char					buf[16][256];
	const char				*lines[16];
	int					n;
	int					i;
	
	if (!load_selected_weapon(selected, sizeof(selected)))
	{
		const char *msg[] = {
			"Aucune arme active.",
			"Va dans 'Choisir une arme active'."
		};
		screen_message(w, "ARME ACTIVE", msg, 2);
		return ;
	}
	if (load_db(&db) != 0)
	{
		const char *msg[] = { "[ERREUR] Impossible de charger armes.ini", tm_path_armes_ini() };
		screen_message(w, "ARME ACTIVE", msg, 2);
		return ;
	}
	wstat = armes_db_find(&db, selected);
	if (!wstat)
	{
		const char *msg[] = {
			"[WARNING] Arme active introuvable.",
			"Verifie le nom exact dans armes.ini."
		};
		armes_db_free(&db);
		screen_message(w, "ARME ACTIVE", msg, 2);
		return ;
	}
	weapon_build_lines(wstat, buf, &n);
	i = 0;
	while (i < n)
	{
		lines[i] = buf[i];
		i++;
	}
	armes_db_free(&db);
	screen_message(w, "ARME ACTIVE", lines, n);
}

static void	screen_weapon_choose(t_window *w)
{
	armes_db			db;
	const char		**items;
	t_menu			m;
	int				action;
	int				i;
	
	if (load_db(&db) != 0)
	{
		const char *msg[] = { "[ERREUR] Impossible de charger armes.ini", tm_path_armes_ini() };
		screen_message(w, "CHOISIR UNE ARME", msg, 2);
		return ;
	}
	if (db.count == 0)
	{
		const char *msg[] = { "Aucune arme dans armes.ini", tm_path_armes_ini() };
		armes_db_free(&db);
		screen_message(w, "CHOISIR UNE ARME", msg, 2);
		return ;
	}
	items = (const char **)calloc(db.count + 1, sizeof(char *));
	if (!items)
	{
		armes_db_free(&db);
		return ;
	}
	i = 0;
	while ((size_t)i < db.count)
	{
		items[i] = db.items[i].name;
		i++;
	}
	items[i] = "Annuler";
	menu_init(&m, items, (int)db.count + 1);
	while (w->running)
	{
		window_poll_events(w);
		if (w->key_escape)
			break ;
		action = menu_update(&m, w);
		if (action >= 0)
		{
			if (action >= 0 && (size_t)action < db.count)
			{
				if (weapon_selected_save(tm_path_weapon_selected(), db.items[action].name) == 0)
				{
					char	line[256];
					const char *msg[2];
					snprintf(line, sizeof(line), "OK : Arme active = %s", db.items[action].name);
					msg[0] = line;
					msg[1] = "(Echap pour revenir)";
					screen_message(w, "CHOISIR UNE ARME", msg, 2);
				}
			}
			break ;
		}
		window_clear(w, 0xFFFFFF);
		window_draw_text(w, 30, 30, "CHOISIR UNE ARME (fleches + Entree)", 0x000000);
		menu_render(&m, w, 30, 80);
		window_present(w);
		ft_sleep_ms(16);
	}
	free(items);
	armes_db_free(&db);
}

static void	screen_stats_once(t_window *w)
{
	long			offset;
	t_hunt_stats	s;
	char			buf[32][256];
	const char		*lines[32];
	int				n;
	int				i;
	
	offset = session_load_offset(tm_path_session_offset());
	memset(&s, 0, sizeof(s));
	if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) != 0)
	{
		const char *msg[] = { "Aucun CSV trouve.", tm_path_hunt_csv(), "Lance le parser LIVE/REPLAY." };
		screen_message(w, "STATS", msg, 3);
		return ;
	}
	n = 0;
	fill_common_header(buf, &n, offset);
	buf[n++][0] = '\0';
	fmt_linef(buf[n++], sizeof(buf[0]), "Kills / Shots", "%ld / %ld", s.kills, s.shots);
	fmt_linef(buf[n++], sizeof(buf[0]), "Loot total", "%.4f PED (%ld PEC)", s.loot_ped, ped_to_pec(s.loot_ped));
	fmt_linef(buf[n++], sizeof(buf[0]), "Depense utilisee", "%.4f PED (%ld PEC)", s.expense_used, ped_to_pec(s.expense_used));
	fmt_linef(buf[n++], sizeof(buf[0]), "Net", "%.4f PED (%ld PEC)", s.net_ped, ped_to_pec(s.net_ped));
	fmt_linef(buf[n++], sizeof(buf[0]), "Return", "%.2f %%", ratio_pct(s.loot_ped, s.expense_used));
	i = 0;
	while (i < n)
	{
		lines[i] = buf[i];
		i++;
	}
	screen_message(w, "STATS (resume)", lines, n);
}

static void	screen_dashboard_live(t_window *w)
{
	t_dash_page	page;
	long		offset;
	t_hunt_stats	s;
	char		buf[32][256];
	const char	*lines[32];
	int		n;
	int		i;
	
	page = DASH_RESUME;
	while (w->running)
	{
		window_poll_events(w);
		if (w->key_escape)
			break ;
		if (w->key_d)
			page = (t_dash_page)((page + 1) % DASH_COUNT);
		else if (w->key_q)
			page = (t_dash_page)((page - 1 + DASH_COUNT) % DASH_COUNT);
		
		offset = session_load_offset(tm_path_session_offset());
		memset(&s, 0, sizeof(s));
		window_clear(w, 0xFFFFFF);
		if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) != 0)
		{
			const char *msg[] = { "Aucun CSV trouve.", tm_path_hunt_csv(), "Lance le parser LIVE/REPLAY.", "(Echap pour quitter)" };
			w_draw_lines(w, 30, 30, msg, 4);
			window_present(w);
			ft_sleep_ms(250);
			continue ;
		}
		n = 0;
		dash_build_lines(&s, offset, page, buf, &n);
		i = 0;
		while (i < n)
		{
			lines[i] = buf[i];
			i++;
		}
		w_draw_lines(w, 30, 30, lines, n);
		window_present(w);
		ft_sleep_ms(250);
	}
}

/* ************************************************************************** */
/*  Actions                                                                   */
/* ************************************************************************** */

static int	export_compute(t_hunt_stats *s, long start_off, long *end_off,
						   char *ts_start, char *ts_end)
{
	if (tracker_stats_compute(tm_path_hunt_csv(), start_off, s) != 0)
		return (0);
	*end_off = session_count_data_lines(tm_path_hunt_csv());
	session_extract_range_timestamps(tm_path_hunt_csv(), start_off,
									 ts_start, 64, ts_end, 64);
	return (1);
}

static void	action_stop_and_export(t_window *w)
{
	t_hunt_stats	export_stats;
	long		start_off;
	long		end_off;
	char		ts_start[64];
	char		ts_end[64];
	char		l1[256];
	char		l2[256];
	char		l3[256];
	const char	*msg[4];
	
	parser_thread_stop();
	start_off = session_load_offset(tm_path_session_offset());
	memset(&export_stats, 0, sizeof(export_stats));
	if (export_compute(&export_stats, start_off, &end_off, ts_start, ts_end))
	{
		if (session_export_stats_csv(tm_path_sessions_stats_csv(),
			&export_stats, ts_start, ts_end))
			snprintf(l1, sizeof(l1), "OK : session exportee dans %s", tm_path_sessions_stats_csv());
		else
			snprintf(l1, sizeof(l1), "[WARN] export session CSV impossible.");
		session_save_offset(tm_path_session_offset(), end_off);
		snprintf(l2, sizeof(l2), "OK : Offset session = %ld ligne(s)", end_off);
		snprintf(l3, sizeof(l3), "Range: %s -> %s", ts_start, ts_end);
		msg[0] = l1;
		msg[1] = l2;
		msg[2] = l3;
		msg[3] = "(Echap pour revenir)";
		screen_message(w, "STOP + EXPORT", msg, 4);
	}
	else
	{
		const char *err[] = { "[ERROR] cannot compute stats for export.", "(Verifie le CSV + l'offset)" };
		screen_message(w, "STOP + EXPORT", err, 2);
	}
}

static void	action_reload_armes(t_window *w)
{
	armes_db	db;
	char		line1[256];
	char		line2[256];
	char		line3[256];
	const char	*msg[3];
	
	if (load_db(&db) != 0)
	{
		const char *err[] = { "[ERREUR] Impossible de charger armes.ini", tm_path_armes_ini() };
		screen_message(w, "RECHARGER ARMES", err, 2);
		return ;
	}
	snprintf(line1, sizeof(line1), "OK : %zu arme(s) chargee(s)", db.count);
	snprintf(line2, sizeof(line2), "Depuis : %s", tm_path_armes_ini());
	if (db.player_name[0])
		snprintf(line3, sizeof(line3), "Joueur : %s", db.player_name);
	else
		line3[0] = '\0';
	msg[0] = line1;
	msg[1] = line2;
	msg[2] = line3;
	armes_db_free(&db);
	screen_message(w, "RECHARGER ARMES", msg, 3);
}

static void	action_toggle_sweat(t_window *w)
{
	int		enabled;
	char		line[128];
	const char	*msg[2];
	
	enabled = 0;
	sweat_option_load(tm_path_options_cfg(), &enabled);
	enabled = !enabled;
	if (sweat_option_save(tm_path_options_cfg(), enabled) != 0)
	{
		const char *err[] = { "[ERREUR] impossible d'ecrire options.cfg", tm_path_options_cfg() };
		screen_message(w, "SWEAT", err, 2);
		return ;
	}
	snprintf(line, sizeof(line), "Tracker Sweat : %s", enabled ? "ON" : "OFF");
	msg[0] = line;
	msg[1] = "(Echap pour revenir)";
	screen_message(w, "SWEAT", msg, 2);
}

static void	action_set_offset_to_end(t_window *w)
{
	long		offset;
	char		line[256];
	const char	*msg[2];
	
	offset = session_count_data_lines(tm_path_hunt_csv());
	session_save_offset(tm_path_session_offset(), offset);
	snprintf(line, sizeof(line), "OK : Offset session = %ld ligne(s) (fin CSV)", offset);
	msg[0] = line;
	msg[1] = "(Echap pour revenir)";
	screen_message(w, "OFFSET", msg, 2);
}

/* ************************************************************************** */
/*  Entry (mode fenetre)                                                      */
/* ************************************************************************** */

void	menu_tracker_chasse(t_window *w)
{
	t_menu		menu;
	int			action;
	const char	*items[11];
	uint64_t	frame_start;
	uint64_t	frame_ms;
	int			sleep_ms;
	
	if (!w)
		return ;
	
	/* Items menu (dernier = retour) */
	items[0] = "Demarrer LIVE (parser chasse)";
	items[1] = "Demarrer REPLAY (parser chasse)";
	items[2] = "Arreter le parser + Export session + Offset";
	items[3] = "Recharger armes.ini";
	items[4] = "Choisir une arme active";
	items[5] = "Afficher l'arme active";
	items[6] = "Afficher les stats (resume)";
	items[7] = "Dashboard LIVE (Q/D pages)";
	items[8] = "Definir offset = fin actuelle du CSV";
	items[9] = "Activer/Desactiver tracker Sweat";
	items[10] = "Retour";
	
	menu_init(&menu, items, 11);
	
	/* Loop “modal” dans la même fenêtre */
	while (w->running)
	{
		frame_start = ft_time_ms();
		window_poll_events(w);
		
		action = menu_update(&menu, w);
		
		/* Escape = menu_update renvoie dernier item => Retour */
		if (action == menu.count - 1)
			break ;
		
		if (action >= 0)
		{
			if (action == 0)
				parser_thread_start_live();
			else if (action == 1)
				parser_thread_start_replay();
			else if (action == 2)
				action_stop_and_export(w);
			else if (action == 3)
				action_reload_armes(w);
			else if (action == 4)
				screen_weapon_choose(w);
			else if (action == 5)
				screen_weapon_active(w);
			else if (action == 6)
				screen_stats_once(w);
			else if (action == 7)
				screen_dashboard_live(w);
			else if (action == 8)
				action_set_offset_to_end(w);
			else if (action == 9)
				action_toggle_sweat(w);
		}
		
		/* Render */
		window_clear(w, 0xFFFFFF);
		window_draw_text(w, 30, 20, "MENU CHASSE (mode fenetre)", 0x000000);
		window_draw_text(w, 30, 45,
						 "Fleches: selection | Entree: valider | Echap: retour",
				   0x000000);
		{
			char st[128];
			snprintf(st, sizeof(st), "Etat parser chasse : %s",
					parser_thread_is_running() ? "EN COURS" : "ARRETE");
			window_draw_text(w, 30, 65, st, 0x000000);
		}
		menu_render(&menu, w, 30, 90);
		window_present(w);
		
		frame_ms = ft_time_ms() - frame_start;
		sleep_ms = 16 - (int)frame_ms;
		if (sleep_ms > 0)
			ft_sleep_ms(sleep_ms);
	}
}

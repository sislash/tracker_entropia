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
	line_h = 12;
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
		window_draw_text(w, 30, 440,
						 "Selectionne 'Confirmer (YES)' pour vider le CSV. (Echap annule)",
						 0x000000);
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
/*  Dashboard LIVE (pages + tops)                                             */
/* ************************************************************************** */

typedef enum e_gdash_page
{
	GDASH_RESUME = 0,
	GDASH_TOP_MOBS,
	GDASH_TOP_CRAFTS,
	GDASH_TOP_RARES,
	GDASH_COUNT
}	t_gdash_page;

static size_t	clamp_top_offset(size_t off, size_t count, size_t page_size)
{
	if (count == 0)
		return (0);
	if (page_size >= count)
		return (0);
	if (off > count - page_size)
		return (count - page_size);
	return (off);
}

static void	build_top_page(const t_globals_top *top, size_t count,
						   const char *title,
						   size_t off, size_t page_size,
						   char buf[64][256], const char **lines, int *out_n)
{
	int		n;
	size_t	i;
	size_t	end;
	
	n = 0;
	snprintf(buf[n++], sizeof(buf[0]), "%s", title);
	buf[n++][0] = '\0';
	if (count == 0)
	{
		snprintf(buf[n++], sizeof(buf[0]), "(Aucun item)");
		*out_n = n;
		i = 0;
		while (i < (size_t)n)
		{
			lines[i] = buf[i];
			i++;
		}
		return ;
	}
	off = clamp_top_offset(off, count, page_size);
	end = off + page_size;
	if (end > count)
		end = count;
	
	snprintf(buf[n++], sizeof(buf[0]),
			 "Items %zu-%zu / %zu  (Z/S scroll)",
			 off + 1, end, count);
	buf[n++][0] = '\0';
	
	i = off;
	while (i < end && n < 62)
	{
		snprintf(buf[n++], sizeof(buf[0]),
				 "%2zu) %-28s  x%ld  %.4f PED",
		   i + 1, top[i].name, top[i].count, top[i].sum_ped);
		i++;
	}
	*out_n = n;
	i = 0;
	while (i < (size_t)n)
	{
		lines[i] = buf[i];
		i++;
	}
}

static void	screen_dashboard_globals_live(t_window *w)
{
	t_globals_stats	s;
	t_gdash_page		page;
	size_t			top_off;
	const size_t		page_size = 10;
	
	char			buf[64][256];
	const char		*lines[64];
	int				n;
	
	page = GDASH_RESUME;
	top_off = 0;
	while (w->running)
	{
		window_poll_events(w);
		if (w->key_escape)
			break ;
		
		/* page switch */
		if (w->key_q)
		{
			page = (t_gdash_page)((page + GDASH_COUNT - 1) % GDASH_COUNT);
			top_off = 0;
		}
		else if (w->key_d)
		{
			page = (t_gdash_page)((page + 1) % GDASH_COUNT);
			top_off = 0;
		}
		
		/* scroll tops */
		if (page != GDASH_RESUME)
		{
			if (w->key_z)
			{
				if (top_off > 0)
					top_off--;
			}
			else if (w->key_s)
				top_off++;
		}
		
		memset(&s, 0, sizeof(s));
		(void)globals_stats_compute(tm_path_globals_csv(), 0, &s);
		
		window_clear(w, 0xFFFFFF);
		
		/* header */
		window_draw_text(w, 30, 20, "DASHBOARD GLOBALS LIVE (fenetre)", 0x000000);
		window_draw_text(w, 30, 45,
						 "Q/D pages | Z/S scroll | Echap quitter",
				   0x000000);
		
		/* page content */
		if (page == GDASH_RESUME)
		{
			n = 0;
			snprintf(buf[n++], sizeof(buf[0]),
					 "[PAGE 1/%d] RESUME", GDASH_COUNT);
			fmt_linef(buf[n++], sizeof(buf[0]), "CSV", "%s", tm_path_globals_csv());
			buf[n++][0] = '\0';
			
			fmt_linef(buf[n++], sizeof(buf[0]), "Lignes lues", "%ld", s.data_lines_read);
			
			buf[n++][0] = '\0';
			fmt_linef(buf[n++], sizeof(buf[0]), "Mob events", "%ld", s.mob_events);
			fmt_linef(buf[n++], sizeof(buf[0]), "Mob sum", "%.4f PED", s.mob_sum_ped);
			fmt_linef(buf[n++], sizeof(buf[0]), "Avg / mob", "%.4f PED",
					  safe_div(s.mob_sum_ped, (double)s.mob_events));
			
			buf[n++][0] = '\0';
			fmt_linef(buf[n++], sizeof(buf[0]), "Craft events", "%ld", s.craft_events);
			fmt_linef(buf[n++], sizeof(buf[0]), "Craft sum", "%.4f PED", s.craft_sum_ped);
			fmt_linef(buf[n++], sizeof(buf[0]), "Avg / craft", "%.4f PED",
					  safe_div(s.craft_sum_ped, (double)s.craft_events));
			
			buf[n++][0] = '\0';
			fmt_linef(buf[n++], sizeof(buf[0]), "Rare events", "%ld", s.rare_events);
			fmt_linef(buf[n++], sizeof(buf[0]), "Rare sum", "%.4f PED", s.rare_sum_ped);
			fmt_linef(buf[n++], sizeof(buf[0]), "Avg / rare", "%.4f PED",
					  safe_div(s.rare_sum_ped, (double)s.rare_events));
			
			buf[n++][0] = '\0';
			fmt_linef(buf[n++], sizeof(buf[0]), "Rare ratio", "%.2f %%",
					  ratio_pct((double)s.rare_events, (double)(s.mob_events + s.craft_events)));
			
			/* map to lines */
			for (int i = 0; i < n; i++)
				lines[i] = buf[i];
			
			w_draw_lines(w, 30, 80, lines, n);
		}
		else if (page == GDASH_TOP_MOBS)
		{
			snprintf(buf[0], sizeof(buf[0]), "[PAGE 2/%d] TOP MOBS", GDASH_COUNT);
			build_top_page(s.top_mobs, s.top_mobs_count,
						   buf[0], top_off, page_size, buf, lines, &n);
			/* clamp top_off after knowing count */
			top_off = clamp_top_offset(top_off, s.top_mobs_count, page_size);
			w_draw_lines(w, 30, 80, lines, n);
		}
		else if (page == GDASH_TOP_CRAFTS)
		{
			snprintf(buf[0], sizeof(buf[0]), "[PAGE 3/%d] TOP CRAFTS", GDASH_COUNT);
			build_top_page(s.top_crafts, s.top_crafts_count,
						   buf[0], top_off, page_size, buf, lines, &n);
			top_off = clamp_top_offset(top_off, s.top_crafts_count, page_size);
			w_draw_lines(w, 30, 80, lines, n);
		}
		else
		{
			snprintf(buf[0], sizeof(buf[0]), "[PAGE 4/%d] TOP RARES", GDASH_COUNT);
			build_top_page(s.top_rares, s.top_rares_count,
						   buf[0], top_off, page_size, buf, lines, &n);
			top_off = clamp_top_offset(top_off, s.top_rares_count, page_size);
			w_draw_lines(w, 30, 80, lines, n);
		}
		
		window_present(w);
		ft_sleep_ms(250);
	}
}

/* ************************************************************************** */
/*  Entry                                                                     */
/* ************************************************************************** */

void	tracker_view_menu_globals(void)
{
	t_window	w;
	t_menu		menu;
	int			action;
	const char	*items[6];
	uint64_t	frame_start;
	uint64_t	frame_ms;
	int			sleep_ms;
	
	if (tm_ensure_logs_dir() != 0)
	{
		/* pas bloquant */
	}
	ensure_globals_csv();
	
	items[0] = "Demarrer LIVE (globals)";
	items[1] = "Demarrer REPLAY (globals)";
	items[2] = "Arreter le parser globals";
	items[3] = "Dashboard LIVE (pages + tops)";
	items[4] = "Vider CSV globals (confirmation YES)";
	items[5] = "Retour";
	
	menu_init(&menu, items, 6);
	if (window_init(&w, "tracker - globals", 900, 650) != 0)
		return ;
	
	while (w.running)
	{
		frame_start = ft_time_ms();
		window_poll_events(&w);
		
		action = menu_update(&menu, &w);
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
					screen_message(&w, "PARSER GLOBALS", msg, 1);
				}
			}
			else if (action == 3)
				screen_dashboard_globals_live(&w);
			else if (action == 4)
				action_clear_globals_csv(&w);
			else
				w.running = 0;
		}
		if (w.key_escape)
			w.running = 0;
		
		window_clear(&w, 0xFFFFFF);
		window_draw_text(&w, 30, 20, "MENU GLOBALS / GLOBAUX (mode fenetre)", 0x000000);
		window_draw_text(&w, 30, 45, "Fleches: selection | Entree: valider | Echap: quitter", 0x000000);
		{
			char st[128];
			snprintf(st, sizeof(st), "Etat parser globals : %s",
					globals_thread_is_running() ? "EN COURS" : "ARRETE");
			window_draw_text(&w, 30, 65, st, 0x000000);
		}
		menu_render(&menu, &w, 30, 90);
		window_present(&w);
		
		frame_ms = ft_time_ms() - frame_start;
		sleep_ms = 16 - (int)frame_ms;
		if (sleep_ms > 0)
			ft_sleep_ms(sleep_ms);
	}
	window_destroy(&w);
}

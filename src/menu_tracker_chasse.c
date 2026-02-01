/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   menu_tracker_chasse.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "menu_tracker_chasse.h"

#include "core_paths.h"
#include "config_arme.h"
#include "parser_thread.h"
#include "session.h"
#include "tracker_stats.h"
#include "tracker_view.h"
#include "ui_key.h"
#include "ui_utils.h"
#include "weapon_selected.h"
#include "menu_principale.h"
#include "sweat_option.h"
#include "session_export.h"

#include <stdio.h>
#include <string.h>

/* ************************************************************************** */
/*  Small helpers                                                             */
/* ************************************************************************** */

static int	read_choice_int(int *out)
{
	int	ret;
	
	if (!out)
		return (0);
	ret = scanf("%d", out);
	ui_flush_stdin();
	return (ret == 1);
}

/* ************************************************************************** */
/*  Weapons helpers                                                           */
/* ************************************************************************** */

static void	print_weapon(const arme_stats *w)
{
	if (!w)
		return;
	printf("\nARME ACTIVE\n");
	print_hr();
	printf("Nom              : %s\n", w->name);
	printf("DPP              : %.4f\n", w->dpp);
	printf("Ammo / tir       : %.6f PED\n", w->ammo_shot);
	printf("Decay / tir      : %.6f PED\n", w->decay_shot);
	printf("Amp decay / tir  : %.6f PED\n", w->amp_decay_shot);
	printf("Markup           : %.4f\n", w->markup);
	printf("Cout / tir total : %.6f PED\n", arme_cost_shot(w));
	if (w->notes[0])
		printf("Notes            : %s\n", w->notes);
	print_hr();
}

static int	load_db(armes_db *db)
{
	if (!db)
		return (-1);
	memset(db, 0, sizeof(*db));
	if (!armes_db_load(db, tm_path_armes_ini()))
	{
		printf("\n[ERREUR] Impossible de charger armes.ini\n");
		printf("Chemin : %s\n\n", tm_path_armes_ini());
		return (-1);
	}
	return (0);
}

static void	menu_weapon_reload(void)
{
	armes_db	db;
	
	if (load_db(&db) != 0)
		return;
	printf("\nOK : %zu arme(s) chargee(s) depuis %s\n",
		   db.count, tm_path_armes_ini());
	if (db.player_name[0])
		printf("Joueur : %s\n", db.player_name);
	armes_db_free(&db);
}

static void	print_no_active_weapon(void)
{
	printf("\nAucune arme active.\n");
	printf("-> Va dans 'Choisir une arme active'.\n\n");
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

static void	menu_weapon_show_active(void)
{
	armes_db		db;
	char			selected[128];
	const arme_stats	*w;
	
	if (!load_selected_weapon(selected, sizeof(selected)))
	{
		print_no_active_weapon();
		return;
	}
	if (load_db(&db) != 0)
		return;
	w = armes_db_find(&db, selected);
	if (!w)
	{
		printf("\n[WARNING] Arme active introuvable : '%s'\n", selected);
		printf("-> Verifie le nom exact dans armes.ini\n\n");
	}
	else
		print_weapon(w);
	armes_db_free(&db);
}

static void	print_weapon_list(const armes_db *db)
{
	size_t	i;
	
	printf("\nLISTE DES ARMES (%zu)\n", db->count);
	print_hr();
	i = 0;
	while (i < db->count)
	{
		printf("%zu) %s\n", i + 1, db->items[i].name);
		i++;
	}
	print_hr();
	printf("0) Annuler\n");
	printf("Choix : ");
}

static int	is_valid_weapon_choice(int choice, size_t count)
{
	if (choice <= 0)
		return (0);
	if ((size_t)choice > count)
		return (0);
	return (1);
}

static void	save_weapon_choice(const armes_db *db, int choice)
{
	const arme_stats	*w;
	
	w = &db->items[(size_t)choice - 1];
	if (weapon_selected_save(tm_path_weapon_selected(), w->name) == 0)
	{
		printf("\nOK : Arme active = %s\n", w->name);
		print_weapon(w);
	}
	else
		printf("\n[ERREUR] Ecriture : %s\n\n", tm_path_weapon_selected());
}

static void	menu_weapon_choose(void)
{
	armes_db	db;
	int		choice;
	
	if (load_db(&db) != 0)
		return;
	if (db.count == 0)
	{
		printf("\nAucune arme dans %s\n\n", tm_path_armes_ini());
		armes_db_free(&db);
		return;
	}
	print_weapon_list(&db);
	if (!read_choice_int(&choice))
		printf("Choix invalide.\n");
	else if (!is_valid_weapon_choice(choice, db.count))
		printf("Annule.\n");
	else
		save_weapon_choice(&db, choice);
	armes_db_free(&db);
}

/* ************************************************************************** */
/*  Sweat option                                                              */
/* ************************************************************************** */

static void	menu_toggle_sweat(void)
{
	int	enabled;
	
	enabled = 0;
	sweat_option_load(tm_path_options_cfg(), &enabled);
	enabled = !enabled;
	if (sweat_option_save(tm_path_options_cfg(), enabled) != 0)
		printf("[ERREUR] impossible d'ecrire options.cfg\n");
	else
		printf("Tracker Sweat : %s\n", enabled ? "ON" : "OFF");
}

/* ************************************************************************** */
/*  Dashboard                                                                 */
/* ************************************************************************** */

static int	dashboard_read_key(void)
{
	if (!ui_key_available())
		return (-1);
	return (ui_key_getch());
}

static void	dashboard_print_ok(long offset, const t_hunt_stats *s)
{
	ui_clear_viewport();
	printf("=== DASHBOARD CHASSE ===\n");
	printf("Parser         : %s\n",
		   parser_thread_is_running() ? "EN COURS" : "ARRETE");
	printf("Offset session : %ld ligne(s)\n", offset);
	printf("CSV            : %s\n\n", tm_path_hunt_csv());
	tracker_view_print(s);
	printf("\n(1-4 pages, A/D changer, Q quitter)\n");
}

static void	dashboard_print_missing_csv(void)
{
	ui_clear_viewport();
	printf("Aucun CSV trouve : %s\n", tm_path_hunt_csv());
	printf("-> Lance le parser LIVE/REPLAY.\n");
	printf("\n(Q pour quitter)\n");
}

static void	dashboard_refresh(void)
{
	long			offset;
	t_hunt_stats	s;
	
	offset = session_load_offset(tm_path_session_offset());
	if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
		dashboard_print_ok(offset, &s);
	else
		dashboard_print_missing_csv();
}

void	menu_dashboard(void)
{
	int	key;
	
	printf("\nDASHBOARD LIVE\n");
	printf("Touches: 1-4 pages, A/D changer, Q pour quitter.\n");
	ui_sleep_ms(250);
	while (1)
	{
		key = dashboard_read_key();
		if (key == 'q' || key == 'Q')
			break;
		if (key > 0)
			tracker_view_handle_key(key);
		dashboard_refresh();
		ui_sleep_ms(150);
	}
}

/* ************************************************************************** */
/*  Menu display                                                              */
/* ************************************************************************** */

static void	print_menu_parser(void)
{
	print_menu_line("Parser (lecture du chat.log)");
	print_menu_line("  1) Demarrer LIVE   (temps reel)");
	print_menu_line("  2) Demarrer REPLAY (relecture)");
	print_menu_line("  3) Arreter le parser (export session + offset)");
	print_hr();
}

static void	print_menu_weapons(void)
{
	print_menu_line("Armes (armes.ini)");
	print_menu_line("  4) Recharger armes.ini (nombre d'armes)");
	print_menu_line("  5) Choisir une arme active");
	print_menu_line("  6) Afficher l'arme active (details + cout/tir)");
	print_hr();
}

static void	print_menu_stats(void)
{
	print_menu_line("Stats & dashboard");
	print_menu_line("  7) Afficher les stats (depuis l'offset de session)");
	print_menu_line("  8) Dashboard LIVE (auto-refresh)");
	print_hr();
}

static void	print_menu_session(void)
{
	print_menu_line("Session");
	print_menu_line("  9) Definir offset = fin actuelle du CSV");
	print_hr();
}

static void	print_menu_sweat(void)
{
	print_menu_line("Sweat ON/OFF");
	print_menu_line(" 10) Activer/Desactiver tracker Sweat");
	print_hr();
}

static void	print_menu(void)
{
	print_hrs();
	print_menu_line("  MENU CHASSE");
	print_hrs();
	print_menu_parser();
	print_menu_weapons();
	print_menu_stats();
	print_menu_session();
	print_menu_sweat();
	print_menu_line("  0) Retour");
	print_hr();
	print_menu_line("Choix : ");
}

/* ************************************************************************** */
/*  Choice handlers                                                           */
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

static void	do_stop_and_export(void)
{
	t_hunt_stats	export_stats;
	long		start_off;
	long		end_off;
	char		ts_start[64];
	char		ts_end[64];
	
	parser_thread_stop();
	start_off = session_load_offset(tm_path_session_offset());
	if (export_compute(&export_stats, start_off, &end_off, ts_start, ts_end))
	{
		if (session_export_stats_csv(tm_path_sessions_stats_csv(),
			&export_stats, ts_start, ts_end))
			printf("OK : session exportee dans %s\n",
				   tm_path_sessions_stats_csv());
			else
				printf("[WARN] export session CSV impossible.\n");
		session_save_offset(tm_path_session_offset(), end_off);
		printf("OK : Offset session = %ld ligne(s)\n", end_off);
	}
	else
		printf("[ERROR] cannot compute stats for export.\n");
	ui_wait_enter();
}

static void	do_show_stats(void)
{
	long			offset;
	t_hunt_stats	s;
	
	offset = session_load_offset(tm_path_session_offset());
	if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
		tracker_view_print(&s);
	else
	{
		printf("\nAucun CSV trouve : %s\n", tm_path_hunt_csv());
		printf("-> Lance le parser LIVE/REPLAY.\n\n");
	}
	ui_wait_enter();
}

static void	do_set_offset_to_end(void)
{
	long	offset;
	
	offset = session_count_data_lines(tm_path_hunt_csv());
	session_save_offset(tm_path_session_offset(), offset);
	printf("\nOK : Offset session = %ld ligne(s) (fin CSV)\n\n", offset);
	ui_wait_enter();
}

static void	handle_choice(int choice)
{
	if (choice == 1)
		parser_thread_start_live();
	else if (choice == 2)
		parser_thread_start_replay();
	else if (choice == 3)
		do_stop_and_export();
	else if (choice == 4)
	{
		menu_weapon_reload();
		ui_wait_enter();
	}
	else if (choice == 5)
	{
		menu_weapon_choose();
		ui_wait_enter();
	}
	else if (choice == 6)
	{
		menu_weapon_show_active();
		ui_wait_enter();
	}
	else if (choice == 7)
		do_show_stats();
	else if (choice == 8)
		menu_dashboard();
	else if (choice == 9)
		do_set_offset_to_end();
	else if (choice == 10)
	{
		menu_toggle_sweat();
		ui_wait_enter();
	}
	else
	{
		printf("Choix inconnu. Entre un nombre (0-10).\n");
		ui_wait_enter();
	}
}

/* ************************************************************************** */
/*  Entry                                                                     */
/* ************************************************************************** */

void	menu_tracker_chasse(void)
{
	int	choice;
	
	if (tm_ensure_logs_dir() != 0)
		printf("[WARN] cannot create logs/\n");
	choice = -1;
	while (choice != 0)
	{
		ui_clear_screen();
		print_status();
		print_menu();
		if (!read_choice_int(&choice))
		{
			printf("Choix invalide. Entre un nombre (0-10).\n");
			continue;
		}
		if (choice != 0)
			handle_choice(choice);
	}
	/* si tu veux forcer l'arret: parser_thread_stop(); */
}

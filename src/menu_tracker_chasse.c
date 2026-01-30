#include "menu_tracker_chasse.h"

#include "core_paths.h"
#include "config_arme.h"
#include "parser_thread.h"
#include "session.h"
#include "tracker_stats.h"
#include "tracker_view.h"
#include "ui_utils.h"
#include "weapon_selected.h"
#include "menu_principale.h"   /* print_status() */
#include "sweat_option.h"
#include "session_export.h"
#include "ui_key.h"   /* ou le header exact de ton module clavier */


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
		return ;
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
		return ;
	printf("\nOK : %zu arme(s) chargee(s) depuis %s\n", db.count, tm_path_armes_ini());
	if (db.player_name[0])
		printf("Joueur : %s\n", db.player_name);
	armes_db_free(&db);
}

static void	menu_weapon_show_active(void)
{
	armes_db			db;
	char				selected[128];
	const arme_stats	*w;
	
	selected[0] = '\0';
	if (weapon_selected_load(tm_path_weapon_selected(), selected,
		sizeof(selected)) != 0 || !selected[0])
	{
		printf("\nAucune arme active.\n");
		printf("-> Va dans 'Choisir une arme active'.\n\n");
		return ;
	}
	if (load_db(&db) != 0)
		return ;
	w = armes_db_find(&db, selected);
	if (!w)
	{
		printf("\n[WARNING] Arme active introuvable dans armes.ini : '%s'\n", selected);
		printf("-> Verifie le nom exact dans la section [Nom Exact]\n\n");
	}
	else
		print_weapon(w);
	armes_db_free(&db);
}

static void	menu_weapon_choose(void)
{
	armes_db	db;
	int			choice;
	
	if (load_db(&db) != 0)
		return ;
	if (db.count == 0)
	{
		printf("\nAucune arme dans %s\n\n", tm_path_armes_ini());
		armes_db_free(&db);
		return ;
	}
	
	printf("\nLISTE DES ARMES (%zu)\n", db.count);
	print_hr();
	for (size_t i = 0; i < db.count; ++i)
		printf("%zu) %s\n", i + 1, db.items[i].name);
	print_hr();
	printf("0) Annuler\n");
	printf("Choix : ");
	
	if (!read_choice_int(&choice))
	{
		printf("Choix invalide.\n");
		armes_db_free(&db);
		return ;
	}
	if (choice <= 0 || (size_t)choice > db.count)
	{
		printf("Annule.\n");
		armes_db_free(&db);
		return ;
	}
	if (weapon_selected_save(tm_path_weapon_selected(),
		db.items[(size_t)choice - 1].name) == 0)
	{
		printf("\nOK : Arme active = %s\n", db.items[(size_t)choice - 1].name);
		print_weapon(&db.items[(size_t)choice - 1]);
	}
	else
		printf("\n[ERREUR] Impossible d'ecrire : %s\n\n", tm_path_weapon_selected());
	
	armes_db_free(&db);
}

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

void	menu_dashboard(void)
{
	long			offset;
	t_hunt_stats	s;
	int				key;
	
	printf("\nDASHBOARD LIVE\n");
	printf("Touches: 1-4 pages, A/D changer page, Q pour quitter.\n");
	ui_sleep_ms(250);
	
	while (1)
	{
		/* -------- clavier non-bloquant -------- */
		key = -1;
		if (ui_key_available())
			key = ui_key_getch();
		
		if (key == 'q' || key == 'Q')
			break;
		if (key > 0)
			tracker_view_handle_key(key);
		
		/* -------- stats + affichage -------- */
		offset = session_load_offset(tm_path_session_offset());
		if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
		{
			ui_clear_viewport();
			printf("=== DASHBOARD CHASSE ===\n");
			printf("Parser         : %s\n",
				   parser_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
			printf("Offset session : %ld ligne(s)\n", offset);
			printf("CSV            : %s\n\n", tm_path_hunt_csv());
			
			/* La view gère désormais les pages */
			tracker_view_print(&s);
			
			printf("\n(1-4 pages, A/D changer, Q quitter)\n");
		}
		else
		{
			ui_clear_viewport();
			printf("Aucun CSV trouve : %s\n", tm_path_hunt_csv());
			printf("-> Lance le parser LIVE/REPLAY pour generer des donnees.\n");
			printf("\n(Q pour quitter)\n");
		}
		
		/*
		 * 800ms = trop lent pour le changement de page.
		 * 150-200ms donne une UX fluide sans surcharger.
		 */
		ui_sleep_ms(150);
	}
}

/* ************************************************************************** */
/*  Menu display                                                              */
/* ************************************************************************** */

static void	print_menu(void)
{
	print_hrs();
	print_menu_line("  MENU CHASSE");
	print_hrs();
	
	print_menu_line("Parser (lecture du chat.log)");
	print_menu_line("  1) Demarrer LIVE   (temps reel)");
	print_menu_line("  2) Demarrer REPLAY (relecture)");
	print_menu_line("  3) Arreter le parser (export session + offset)");
	print_hr();
	
	print_menu_line("Armes (armes.ini)");
	print_menu_line("  4) Recharger armes.ini (affiche le nombre d'armes)");
	print_menu_line("  5) Choisir une arme active");
	print_menu_line("  6) Afficher l'arme active (details + cout/tir)");
	print_hr();
	
	print_menu_line("Stats & dashboard");
	print_menu_line("  7) Afficher les stats (depuis l'offset de session)");
	print_menu_line("  8) Dashboard LIVE (auto-refresh)");
	print_hr();
	
	print_menu_line("Session");
	print_menu_line("  9) Definir offset = fin actuelle du CSV");
	print_hr();
	
	print_menu_line("Sweat ON/OFF");
	print_menu_line(" 10) Activer/Desactiver tracker Sweat");
	print_hr();
	
	print_menu_line("  0) Retour");
	print_hr();
	print_menu_line("Choix : ");
}

/* ************************************************************************** */
/*  Entry                                                                     */
/* ************************************************************************** */

void	menu_tracker_chasse(void)
{
	int				choice;
	long			offset;
	t_hunt_stats		s;
	
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
			continue ;
		}
		
		if (choice == 1)
			parser_thread_start_live();
		else if (choice == 2)
			parser_thread_start_replay();
		else if (choice == 3)
		{
			t_hunt_stats	export_stats;
			long			start_off;
			long			end_off;
			char			ts_start[64];
			char			ts_end[64];
			
			parser_thread_stop();
			
			start_off = session_load_offset(tm_path_session_offset());
			if (tracker_stats_compute(tm_path_hunt_csv(), start_off, &export_stats) == 0)
			{
				end_off = session_count_data_lines(tm_path_hunt_csv());
				session_extract_range_timestamps(tm_path_hunt_csv(), start_off,
												 ts_start, sizeof(ts_start), ts_end, sizeof(ts_end));
				
				if (session_export_stats_csv(tm_path_sessions_stats_csv(),
					&export_stats, ts_start, ts_end))
					printf("OK : session exportee dans %s\n", tm_path_sessions_stats_csv());
				else
					printf("[WARN] export session CSV impossible.\n");
				
				session_save_offset(tm_path_session_offset(), end_off);
				printf("OK : Offset session = %ld ligne(s)\n", end_off);
			}
			else
				printf("[ERROR] cannot compute stats for export.\n");
			
			ui_wait_enter();
		}
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
		{
			offset = session_load_offset(tm_path_session_offset());
			if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
			{
				tracker_view_print(&s);
				ui_wait_enter();
			}
			else
			{
				printf("\nAucun CSV trouve : %s\n", tm_path_hunt_csv());
				printf("-> Lance le parser LIVE/REPLAY.\n\n");
				ui_wait_enter();
			}
		}
		else if (choice == 8)
			menu_dashboard();
		else if (choice == 9)
		{
			offset = session_count_data_lines(tm_path_hunt_csv());
			session_save_offset(tm_path_session_offset(), offset);
			printf("\nOK : Offset session = %ld ligne(s) (fin CSV)\n\n", offset);
			ui_wait_enter();
		}
		else if (choice == 10)
		{
			menu_toggle_sweat();
			ui_wait_enter();
		}
		else if (choice != 0)
		{
			printf("Choix inconnu. Entre un nombre (0-10).\n");
			ui_wait_enter();
		}
	}
	/* IMPORTANT : on ne force pas stop ici, mais si tu veux:
	 *	   parser_thread_stop(); */
}

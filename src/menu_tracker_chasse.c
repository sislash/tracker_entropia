#include "menu_tracker_chasse.h"

#include "core_paths.h"
#include "config_arme.h"
#include "parser_thread.h"
#include "session.h"
#include "tracker_stats.h"
#include "tracker_view.h"
#include "ui_utils.h"
#include "weapon_selected.h"
#include "menu_principale.h"
#include "sweat_option.h"


#include <stdio.h>
#include <string.h>

/* ************************************************************************** */
/*  Small helpers                                                             */
/* ************************************************************************** */

static void	print_hr(void)
{
	printf("------------------------------------------------------------\n");
}

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
	{
		printf("\n[ERREUR] Impossible d'ecrire : %s\n\n", tm_path_weapon_selected());
	}
	armes_db_free(&db);
}

static void menu_toggle_sweat(void)
{
	int enabled = 0;
	
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
	t_hunt_stats		s;
	
	printf("\nDASHBOARD LIVE\n");
	printf("Appuie sur 'q' pour quitter.\n");
	ui_sleep_ms(250);
	
	while (!ui_user_wants_quit())
	{
		offset = session_load_offset(tm_path_session_offset());
		if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
		{
			ui_clear_viewport();
			printf("=== DASHBOARD CHASSE ===\n");
			printf("Parser         : %s\n",
				   parser_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
			printf("Offset session : %ld ligne(s)\n", offset);
			printf("CSV            : %s\n\n", tm_path_hunt_csv());
			tracker_view_print(&s);
			printf("\n(press 'q' to quit)\n");
		}
		else
		{
			ui_clear_viewport();
			printf("Aucun CSV trouve : %s\n", tm_path_hunt_csv());
			printf("-> Lance le parser LIVE/REPLAY pour generer des donnees.\n");
			printf("\n(press 'q' to quit)\n");
		}
		ui_sleep_ms(800);
	}
}

/* ************************************************************************** */
/*  Menu display                                                              */
/* ************************************************************************** */

static void	print_menu(void)
{
	printf("\n");
	printf("============================================================\n");
	printf("  MENU CHASSE\n");
	printf("============================================================\n");
	
	printf("Parser (lecture du chat.log)\n");
	printf("  1) Demarrer LIVE   (temps reel)\n");
	printf("  2) Demarrer REPLAY (relecture)\n");
	printf("  3) Arreter le parser\n");
	printf("\n");
	
	printf("Armes (armes.ini)\n");
	printf("  4) Recharger armes.ini (affiche le nombre d'armes)\n");
	printf("  5) Choisir une arme active\n");
	printf("  6) Afficher l'arme active (details + cout/tir)\n");
	printf("\n");
	
	printf("Stats & dashboard\n");
	printf("  7) Afficher les stats (depuis l'offset de session)\n");
	printf("  8) Dashboard LIVE (auto-refresh)\n");
	printf("\n");
	
	printf("Session\n");
	printf("  9) Definir offset = fin actuelle du CSV\n");
	printf("\n");
	
	printf("Sweat ON/OFF\n");
	printf(" 10) Activer/Desactiver tracker Sweat\n");
	printf("\n");
	
	printf("  0) Retour\n");
	printf("\n");
	printf("Choix : ");
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
			parser_thread_stop();
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
	
	parser_thread_stop();
}

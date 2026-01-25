#include "menu_principale.h"

#include "core_paths.h"
#include "fs_utils.h"
#include "config_arme.h"

#include "menu_tracker_chasse.h"
#include "parser_thread.h"
#include "globals_thread.h"

#include "globals_stats.h"
#include "globals_view.h"

#include "session.h"
#include "tracker_stats.h"
#include "tracker_view.h"
#include "ui_utils.h"
#include "weapon_selected.h"
#include "csv.h"
#include "session_export.h"
#include "ui_key.h"

#include <stdio.h>
#include <string.h>

/* ************************************************************************** */
/*  Helpers                                                                   */
/* ************************************************************************** */

static void	init_storage(void)
{
	FILE	*f;
	
	tm_ensure_logs_dir();
	
	f = fopen(tm_path_hunt_csv(), "ab");
	if (f)
	{
		csv_ensure_header6(f);
		fclose(f);
	}
	f = fopen(tm_path_globals_csv(), "ab");
	if (f)
	{
		csv_ensure_header6(f);
		fclose(f);
	}
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

static void	print_hr(void)
{
	printf("|---------------------------------------------------------------------------|\n");
}

/* ************************************************************************** */
/*  Status                                                                    */
/* ************************************************************************** */

void	print_status(void)
{
	char				weapon[128];
	long				offset;
	int					ini_ok;
	armes_db			db;
	const arme_stats	*w;
	double				cost_shot;
	
	weapon[0] = '\0';
	(void)weapon_selected_load(tm_path_weapon_selected(), weapon, sizeof(weapon));
	offset = session_load_offset(tm_path_session_offset());
	
	printf("\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|===========================================================================|\n");
	printf("|  TRACKER MODULAIRE  |  Entropia Universe (C99)                            |\n");
	printf("|===========================================================================|\n");
	
	printf("|Parser CHASSE    : %s                                        |\n",
		   parser_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
	printf("|Parser GLOBALS   : %s                                        |\n",
		   globals_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
	
	printf("|Arme active      : %s                       |\n", weapon[0] ? weapon : "(aucune)");
	printf("|Offset session   : %ld ligne(s) de donnees                                   |\n", offset);
	printf("|CSV hunt         : %s                                       |\n", tm_path_hunt_csv());
	printf("|CSV globals      : %s                                       |\n", tm_path_globals_csv());
	printf("|Armes config     : %s                                               |\n", tm_path_armes_ini());
	print_hr();
	
	ini_ok = fs_file_exists(tm_path_armes_ini());
	if (!ini_ok)
	{
		printf("⚠ WARNING : Fichier armes.ini introuvable.\n");
		printf("  -> Cree/Place le fichier ici : %s\n", tm_path_armes_ini());
		print_hr();
		return ;
	}
	if (!weapon[0])
	{
		printf("ℹ Astuce  : Selectionne une arme (Menu Chasse) pour calculer les couts.\n");
		print_hr();
		return ;
	}
	memset(&db, 0, sizeof(db));
	if (armes_db_load(&db, tm_path_armes_ini()) == 0)
	{
		printf("⚠ WARNING : Impossible de lire armes.ini (format/acces).\n");
		print_hr();
		return ;
	}
	w = armes_db_find(&db, weapon);
	if (!w)
	{
		printf("⚠ WARNING : L'arme active n'existe pas dans armes.ini.\n");
		printf("  -> Verifie l'orthographe du nom (section [Nom Exact]).\n");
	}
	else
	{
		cost_shot = arme_cost_shot(w);
		printf("|Cout par tir     : %.6f PED                                            |\n", cost_shot);
	}
	armes_db_free(&db);
	print_hr();
}

/* ************************************************************************** */
/*  Menus                                                                      */
/* ************************************************************************** */

static void	print_menu(void)
{
	printf("|---------------------------------------------------------------------------|\n");
	printf("|       ####### ###    ## ######## ####### ####### ####### ## #######       |\n");
	printf("|       ##      ## #   ##    ##    ##   ## ##   ## ##   ## ## ##   ##       |\n");
	printf("|       #####   ##  #  ##    ##    ####### ##   ## ####### ## #######       |\n");
	printf("|       ##      ##   # ##    ##    ## ##   ##   ## ##      ## ##   ##       |\n");
	printf("|       ####### ##    ###    ##    ##   ## ####### ##      ## ##   ##       |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|                             MENU PRINCIPAL                                |\n");
	printf("|                                                                           |\n");
	printf("|Chasse                                                                     |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  1) Menu Chasse (armes, dashboard, options chasse)                        |\n");
	printf("|  2) Demarrer parser CHASSE LIVE                                           |\n");
	printf("|  3) Demarrer parser CHASSE REPLAY                                         |\n");
	printf("|  4) Arreter parser CHASSE (export session + offset)                       |\n");
	printf("|                                                                           |\n");
	printf("|Globals / HOF / ATH (MOB + CRAFT)                                          |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("| 10) Demarrer parser GLOBALS LIVE                                          |\n");
	printf("| 11) Demarrer parser GLOBALS REPLAY                                        |\n");
	printf("| 12) Arreter parser GLOBALS                                                |\n");
	printf("| 13) Dashboard GLOBALS LIVE (separe)                                       |\n");
	printf("| 14) Vider CSV GLOBALS (demande confirmation)                              |\n");
	printf("|                                                                           |\n");
	printf("|Stats chasse                                                               |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  5) Afficher stats chasse (depuis offset session)                         |\n");
	printf("|  6) Dashboard chasse LIVE (auto-refresh)                                  |\n");
	printf("|                                                                           |\n");
	printf("|Session chasse                                                             |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  7) Offset = fin actuelle du CSV                                          |\n");
	printf("|  8) Reset offset = 0                                                      |\n");
	printf("|  9) Vider CSV hunt (demande confirmation)                                 |\n");
	printf("|                                                                           |\n");
	printf("|  0) Quitter                                                               |\n");
	printf("|                                                                           |\n");
	printf("|Choix :                                                                    |\n");
	printf("|___________________________________________________________________________|\n");
}

/* ************************************************************************** */
/*  Actions                                                                    */
/* ************************************************************************** */

static void	menu_show_stats_hunt(void)
{
	long			offset;
	t_hunt_stats		s;
	
	memset(&s, 0, sizeof(s));
	offset = session_load_offset(tm_path_session_offset());
	if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) != 0)
	{
		printf("[ERROR] cannot compute hunt stats.\n");
		ui_wait_enter();
		return ;
	}
	tracker_view_print(&s);
	ui_wait_enter();
}

static void	menu_dashboard_hunt_live(void)
{
	/* tu as deja un dashboard dans menu_tracker_chasse normalement,
	 *	   mais tu voulais le garder aussi ici => on appelle celui existant */
	menu_dashboard();
}

static void	menu_dashboard_globals_live(void)
{
	t_globals_stats	s;
	
	while (1)
	{
		memset(&s, 0, sizeof(s));
		(void)globals_stats_compute(tm_path_globals_csv(), 0, &s);
		globals_view_print(&s);
		
		if (ui_key_available())
		{
			int c = ui_key_getch();
			if (c == 'q' || c == 'Q' || c == 27)
				break;
		}
		ui_sleep_ms(250);
	}
}

static void	menu_clear_csv_hunt(void)
{
	char	confirm[8];
	FILE	*f;
	
	printf("\nATTENTION : tu vas vider le fichier :\n%s\n", tm_path_hunt_csv());
	printf("Tape YES pour confirmer : ");
	if (scanf("%7s", confirm) != 1)
	{
		ui_flush_stdin();
		printf("Annule.\n");
		return ;
	}
	ui_flush_stdin();
	if (strcmp(confirm, "YES") != 0)
	{
		printf("Annule.\n");
		return ;
	}
	f = fopen(tm_path_hunt_csv(), "w");
	if (!f)
	{
		printf("[ERREUR] Impossible d'ouvrir le CSV en ecriture.\n");
		return ;
	}
	fclose(f);
	
	f = fopen(tm_path_hunt_csv(), "ab");
	if (f)
	{
		csv_ensure_header6(f);
		fclose(f);
	}
	printf("OK : CSV hunt vide.\n");
	session_save_offset(tm_path_session_offset(), 0);
	printf("OK : Offset session remis a 0.\n");
}

static void	menu_clear_csv_globals(void)
{
	char	confirm[8];
	FILE	*f;
	
	printf("\nATTENTION : tu vas vider le fichier :\n%s\n", tm_path_globals_csv());
	printf("Tape YES pour confirmer : ");
	if (scanf("%7s", confirm) != 1)
	{
		ui_flush_stdin();
		printf("Annule.\n");
		return ;
	}
	ui_flush_stdin();
	if (strcmp(confirm, "YES") != 0)
	{
		printf("Annule.\n");
		return ;
	}
	f = fopen(tm_path_globals_csv(), "w");
	if (!f)
	{
		printf("[ERREUR] Impossible d'ouvrir le CSV en ecriture.\n");
		return ;
	}
	fclose(f);
	
	f = fopen(tm_path_globals_csv(), "ab");
	if (f)
	{
		csv_ensure_header6(f);
		fclose(f);
	}
	printf("OK : CSV globals vide.\n");
}

/* ************************************************************************** */
/*  Stop CHASSE + export session                                               */
/* ************************************************************************** */

static void	stop_hunt_export_and_offset(void)
{
	t_hunt_stats	s;
	long			start_off;
	long			end_off;
	char			ts_start[64];
	char			ts_end[64];
	
	parser_thread_stop();
	
	start_off = session_load_offset(tm_path_session_offset());
	if (tracker_stats_compute(tm_path_hunt_csv(), start_off, &s) == 0)
	{
		end_off = session_count_data_lines(tm_path_hunt_csv());
		session_extract_range_timestamps(tm_path_hunt_csv(), start_off,
										 ts_start, sizeof(ts_start), ts_end, sizeof(ts_end));
		
		if (session_export_stats_csv(tm_path_sessions_stats_csv(), &s, ts_start, ts_end))
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

/* ************************************************************************** */
/*  Entry point                                                               */
/* ************************************************************************** */

void	menu_principale(void)
{
	int		choice;
	long	offset;
	
	init_storage();
	
	choice = -1;
	while (choice != 0)
	{
		ui_clear_screen();
		print_status();
		print_menu();
		
		if (!read_choice_int(&choice))
		{
			printf("Choix invalide.\n");
			continue ;
		}
		
		if (choice == 1)
			menu_tracker_chasse();
		else if (choice == 2)
			parser_thread_start_live();
		else if (choice == 3)
			parser_thread_start_replay();
		else if (choice == 4)
			stop_hunt_export_and_offset();
		
		else if (choice == 5)
			menu_show_stats_hunt();
		else if (choice == 6)
			menu_dashboard_hunt_live();
		
		else if (choice == 7)
		{
			offset = session_count_data_lines(tm_path_hunt_csv());
			session_save_offset(tm_path_session_offset(), offset);
			printf("OK : Offset session = %ld ligne(s) (fin CSV)\n", offset);
			ui_wait_enter();
		}
		else if (choice == 8)
		{
			session_save_offset(tm_path_session_offset(), 0);
			printf("OK : Offset session remis a 0\n");
			ui_wait_enter();
		}
		else if (choice == 9)
		{
			menu_clear_csv_hunt();
			ui_wait_enter();
		}
		
		else if (choice == 10)
			globals_thread_start_live();
		else if (choice == 11)
			globals_thread_start_replay();
		else if (choice == 12)
		{
			globals_thread_stop();
			printf("OK : parser GLOBALS arrete.\n");
			ui_wait_enter();
		}
		else if (choice == 13)
			menu_dashboard_globals_live();
		else if (choice == 14)
		{
			menu_clear_csv_globals();
			ui_wait_enter();
		}
		else if (choice != 0)
		{
			printf("Choix inconnu.\n");
			ui_wait_enter();
		}
	}
	
	parser_thread_stop();
	globals_thread_stop();
}

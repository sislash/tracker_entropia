
#include "menu_principale.h"

#include "core_paths.h"
#include "fs_utils.h"
#include "config_arme.h"
#include "menu_tracker_chasse.h"
#include "parser_thread.h"
#include "globals_thread.h"
#include "menu_globals.h"
#include "session.h"
#include "ui_utils.h"
#include "weapon_selected.h"
#include "csv.h"

#include <stdio.h>
#include <string.h>

/* ************************************************************************** */
/*  Helpers                                                                   */
/* ************************************************************************** */

static void	init_storage(void)
{
	FILE	*f;
	
	tm_ensure_logs_dir();
	
	/* Hunt CSV */
	f = fopen(tm_path_hunt_csv(), "ab");
	if (f)
	{
		csv_ensure_header6(f);
		fclose(f);
	}
	
	/* Globals CSV */
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

void print_status(void)
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
	
	print_status_line("Parser CHASSE", parser_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
	print_status_line("Parser GLOBALS", globals_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
	print_status_line("Arme active", weapon[0] ? weapon : "(aucune)");
	print_status_linef("Session (offset)", "%ld ligne(s) de donnees", offset);
	print_status_line("CSV hunt", tm_path_hunt_csv());
	print_status_line("CSV globals", tm_path_globals_csv());
	print_status_line("Armes config", tm_path_armes_ini());
	print_status_line("markup config", tm_path_markup_ini());
	
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
		printf("ℹ Astuce  : Selectionne une arme (dans MENU CHASSE) pour calculer les couts.\n");
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
		print_status_linef("Cout par tir","%.6f PED", cost_shot);
	}
	armes_db_free(&db);
	print_hr();
}

/* ************************************************************************** */
/*  Menu                                                                      */
/* ************************************************************************** */

static void	print_menu(void)
{
	printf("|---------------------------------------------------------------------------|\n");
	printf("|       ####### ###    ## ######## ####### ####### ####### ## #######       |\n");
	printf("|       ##      ## #   ##    ##    ##   ## ##   ## ##   ## ## ##   ##       |\n");
	printf("|       #####   ##  #  ##    ##    ####### ##   ## ####### ## #######       |\n");
	printf("|       ##      ##   # ##    ##    ## ##   ##   ## ##      ## ##   ##       |\n");
	printf("|       ####### ##    ###    ##    ##   ## ####### ##      ## ##   ##       |\n");
	printf("|                                                                           |\n");
	printf("|                 #  # ##   # # #   # #### #### #### ####                   |\n");
	printf("|                 #  # # #  # # #   # #    #  # #    #                      |\n");
	printf("|=====            #  # # ## # # #   # ###  #### #### ###               =====|\n");
	printf("|                 #  # #  # # #  # #  #    # #     # #                      |\n");
	printf("|                 #### #   ## #   #   #### #  # #### ####                   |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|                                                                           |\n");
	printf("|                             MENU PRINCIPAL                                |\n");
	printf("|                                                                           |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  1) Menu CHASSE (tout: parser, armes, stats, dashboard, sweat)            |\n");
	printf("|  2) Menu GLOBALS (parser + dashboard separe + CSV)                        |\n");
	printf("|                                                                           |\n");
	printf("|  9) STOP ALL (arreter tous les parsers)                                   |\n");
	printf("|                                                                           |\n");
	printf("|  0) Quitter                                                               |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|Choix :                                                                    |");
}

/* ************************************************************************** */
/*  Entry point                                                               */
/* ************************************************************************** */

void	menu_principale(void)
{
	int	choice;
	
	init_storage();
	if (tm_ensure_logs_dir() != 0)
		printf("[WARN] impossible de creer logs/\n");
	
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
			menu_globals();
		else if (choice == 9)
		{
			parser_thread_stop();
			globals_thread_stop();
			printf("\nOK : tous les parsers sont arretes.\n\n");
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

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   menu_principale.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "menu_principale.h"

#include "config_arme.h"
#include "core_paths.h"
#include "csv.h"
#include "fs_utils.h"
#include "globals_thread.h"
#include "menu_globals.h"
#include "menu_tracker_chasse.h"
#include "parser_thread.h"
#include "session.h"
#include "ui_utils.h"
#include "weapon_selected.h"

#include <stdio.h>
#include <string.h>

static void	print_ln(const char *s)
{
	printf("%s\n", s);
}

static void	init_storage(void)
{
	FILE	*f;
	
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

static void	print_title_bar(void)
{
	print_ln("|============================"
	"===========================================|");
	print_ln("|  TRACKER MODULAIRE  |  Entropia Universe (C99)"
	"                            |");
	print_ln("|============================"
	"===========================================|");
}

static const char	*run_state(int running)
{
	if (running)
		return ("EN COURS (RUNNING)");
	return ("ARRETE (STOPPED)");
}

static int	init_status(char *weapon, long *offset)
{
	if (!weapon || !offset)
		return (0);
	weapon[0] = '\0';
	(void)weapon_selected_load(tm_path_weapon_selected(), weapon, 128);
	*offset = session_load_offset(tm_path_session_offset());
	return (1);
}

static int	print_status_warn_no_ini(void)
{
	printf("⚠ WARNING : Fichier armes.ini introuvable.\n");
	printf("  -> Cree/Place le fichier ici : %s\n", tm_path_armes_ini());
	print_hr();
	return (0);
}

static int	print_status_warn_no_weapon(void)
{
	printf("ℹ Astuce  : Selectionne une arme (dans MENU CHASSE)"
	" pour calculer les couts.\n");
	print_hr();
	return (0);
}

static int	armes_db_load_safe(armes_db *db)
{
	memset(db, 0, sizeof(*db));
	if (armes_db_load(db, tm_path_armes_ini()) == 0)
	{
		printf("⚠ WARNING : Impossible de lire armes.ini (format/acces).\n");
		print_hr();
		return (0);
	}
	return (1);
}

static void	print_cost_shot(const char *weapon)
{
	armes_db			db;
	const arme_stats	*w;
	double				cost_shot;
	
	if (!armes_db_load_safe(&db))
		return;
	w = armes_db_find(&db, weapon);
	if (!w)
	{
		printf("⚠ WARNING : L'arme active n'existe pas dans armes.ini.\n");
		printf("  -> Verifie l'orthographe du nom (section [Nom Exact]).\n");
	}
	else
	{
		cost_shot = arme_cost_shot(w);
		print_status_linef("Cout par tir", "%.6f PED", cost_shot);
	}
	armes_db_free(&db);
	print_hr();
}

static void	print_status_lines(const char *weapon, long offset)
{
	print_title_bar();
	print_status_line("Parser CHASSE", run_state(parser_thread_is_running()));
	print_status_line("Parser GLOBALS", run_state(globals_thread_is_running()));
	print_status_line("Arme active", weapon);
	print_status_linef("Session (offset)", "%ld ligne(s) de donnees", offset);
	print_status_line("CSV hunt", tm_path_hunt_csv());
	print_status_line("CSV globals", tm_path_globals_csv());
	print_status_line("Armes config", tm_path_armes_ini());
	print_status_line("markup config", tm_path_markup_ini());
	print_hr();
}

void	print_status(void)
{
	char		weapon[128];
	long		offset;
	const char	*weapon_label;
	
	if (!init_status(weapon, &offset))
		return;
	weapon_label = "(aucune)";
	if (weapon[0])
		weapon_label = weapon;
	print_status_lines(weapon_label, offset);
	if (!fs_file_exists(tm_path_armes_ini()))
	{
		print_status_warn_no_ini();
		return;
	}
	if (!weapon[0])
	{
		print_status_warn_no_weapon();
		return;
	}
	print_cost_shot(weapon);
}

static void	print_menu_art_top(void)
{
	print_ln("|--------------------------------"
	"-------------------------------------------|");
	print_ln("|       ####### ###    ## ######## ####### ####### ####### ##"
	" #######       |");
	print_ln("|       ##      ## #   ##    ##    ##   ## ##   ## ##   ## ##"
	" ##   ##       |");
	print_ln("|       #####   ##  #  ##    ##    ####### ##   ## ####### ##"
	" #######       |");
	print_ln("|       ##      ##   # ##    ##    ## ##   ##   ## ##      ##"
	" ##   ##       |");
	print_ln("|       ####### ##    ###    ##    ##   ## ####### ##      ##"
	" ##   ##       |");
}

static void	print_menu_art_bottom(void)
{
	print_ln("|"
	"                                                                           "
	"|");
	print_ln("|                 #  # ##   # # #   # #### #### #### ####"
	"                   |");
	print_ln("|                 #  # # #  # # #   # #    #  # #"
	"    #                      |");
	print_ln("|=====            #  # # ## # # #   # ###  #### #### ###"
	"               =====|");
	print_ln("|                 #  # #  # # #  # #  #    # #"
	"     # #                      |");
	print_ln("|                 #### #   ## #   #   #### #  # #### ####"
	"                   |");
	print_ln("|--------------------------------"
	"-------------------------------------------|");
}

static void	print_menu_art(void)
{
	print_menu_art_top();
	print_menu_art_bottom();
}

static void	print_menu_options_header(void)
{
	print_ln("|"
	"                                                                           "
	"|");
	print_ln("|                             MENU PRINCIPAL"
	"                                |");
	print_ln("|"
	"                                                                           "
	"|");
	print_ln("|--------------------------------"
	"-------------------------------------------|");
}

static void	print_menu_options_body(void)
{
	print_ln("|  1) Menu CHASSE (tout: parser, armes, stats, dashboard, sweat)"
	"            |");
	print_ln("|  2) Menu GLOBALS (parser + dashboard separe + CSV)"
	"                        |");
	print_ln("|"
	"                                                                           "
	"|");
	print_ln("|  9) STOP ALL (arreter tous les parsers)"
	"                                   |");
	print_ln("|"
	"                                                                           "
	"|");
	print_ln("|  0) Quitter"
	"                                                               |");
	print_ln("|--------------------------------"
	"-------------------------------------------|");
	print_ln("|Choix :"
	"                                                                    |");
}

static void	print_menu_options(void)
{
	print_menu_options_header();
	print_menu_options_body();
}

static void	print_menu(void)
{
	print_menu_art();
	print_menu_options();
}

static void	stop_all_parsers(void)
{
	parser_thread_stop();
	globals_thread_stop();
	printf("\nOK : tous les parsers sont arretes.\n\n");
	ui_wait_enter();
}

static void	handle_choice(int choice)
{
	if (choice == 1)
		menu_tracker_chasse();
	else if (choice == 2)
		menu_globals();
	else if (choice == 9)
		stop_all_parsers();
	else if (choice != 0)
	{
		printf("Choix inconnu.\n");
		ui_wait_enter();
	}
}

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
			continue;
		}
		handle_choice(choice);
	}
	parser_thread_stop();
	globals_thread_stop();
}

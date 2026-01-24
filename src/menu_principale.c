#include "menu_principale.h"

#include "core_paths.h"
#include "fs_utils.h"
#include "config_arme.h"
#include "menu_tracker_chasse.h"
#include "parser_thread.h"
#include "session.h"
#include "tracker_stats.h"
#include "tracker_view.h"
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
	f = fopen(tm_path_hunt_csv(), "ab");
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
	printf("------------------------------------------------------------\n");
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
	
	/* Tu peux décommenter si tu veux un écran plus “propre” à chaque boucle */
	/* ui_clear_screen(); */
	
	printf("\n");
	printf("============================================================\n");
	printf("  TRACKER MODULAIRE  |  Entropia Universe (C99)\n");
	printf("============================================================\n");
	
	printf("Etat parser      : %s\n",
		   parser_thread_is_running() ? "EN COURS (RUNNING)" : "ARRETE (STOPPED)");
	printf("Arme active      : %s\n", weapon[0] ? weapon : "(aucune)");
	printf("Session (offset) : %ld ligne(s) de donnees\n", offset);
	printf("CSV log          : %s\n", tm_path_hunt_csv());
	printf("Armes config     : %s\n", tm_path_armes_ini());
	print_hr();
	
	/* Aide "intelligente" : coût/tir + warnings simples */
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
		printf("ℹ Astuce  : Selectionne une arme (menu 1) pour calculer les couts.\n");
		print_hr();
		return ;
	}
	memset(&db, 0, sizeof(db));
	if (armes_db_load(&db, tm_path_armes_ini()) == 0)
	{
		printf("⚠ WARNING : Impossible de lire armes.ini (format/accès).\n");
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
		printf("Cout par tir     : %.6f PED\n", cost_shot);
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
	printf("|                             MENU PRINCIPAL                                |\n");
	printf("|                                                                           |\n");
	printf("|Actions principales                                                        |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  1) Menu Chasse (armes, dashboard, options chasse)                        |\n");
	printf("|  2) Demarrer le parser en LIVE   (temps reel sur chat.log)                |\n");
	printf("|  3) Demarrer le parser en REPLAY (relire un chat.log deja rempli)         |\n");
	printf("|  4) Arreter le parser                                                     |\n");
	printf("|                                                                           |\n");
	printf("|Statistiques                                                               |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  5) Afficher les stats (depuis l'offset de session)                       |\n");
	printf("|  6) Dashboard LIVE (auto-refresh)                                         |\n");
	printf("|                                                                           |\n");
	printf("|Session & fichiers                                                         |\n");
	printf("|---------------------------------------------------------------------------|\n");
	printf("|  7) Definir l'offset = fin actuelle du CSV (reprendre a zero visuellement)|\n");
	printf("|  8) Reset offset = 0 (recalculer stats depuis le debut)                   |\n");
	printf("|  9) Vider le CSV de hunt (demande confirmation)                           |\n");
	printf("|                                                                           |\n");
	printf("|  0) Quitter                                                               |\n");
	printf("|                                                                           |\n");
	printf("|Choix :                                                                    |\n");
	printf("|___________________________________________________________________________|\n");
}

/* ************************************************************************** */
/*  Actions                                                                   */
/* ************************************************************************** */

static void	menu_show_stats(void)
{
	long			offset;
	t_hunt_stats		s;
	
	memset(&s, 0, sizeof(s));
	offset = session_load_offset(tm_path_session_offset());
	if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) != 0)
	{
		printf("[ERROR] cannot compute stats (missing CSV?)\n");
		ui_wait_enter();
		return ;
	}
	tracker_view_print(&s);
	
	ui_wait_enter();
}

static void	menu_clear_csv(void)
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
	printf("OK : CSV vide.\n");
	session_save_offset(tm_path_session_offset(), 0);
	printf("OK : Offset session remis a 0.\n");
}

/* ************************************************************************** */
/*  Entry point                                                               */
/* ************************************************************************** */

void	menu_principale(void)
{
	int		choice;
	long		offset;
	
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
			printf("Choix invalide. Entre un nombre (0-9).\n");
			continue ;
		}
		
		if (choice == 1)
			menu_tracker_chasse();
		else if (choice == 2)
			parser_thread_start_live();
		else if (choice == 3)
			parser_thread_start_replay();
		else if (choice == 4)
			parser_thread_stop();
		else if (choice == 5)
			menu_show_stats();
		else if (choice == 6)
			menu_dashboard();
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
			menu_clear_csv();
			ui_wait_enter();
		}
		else if (choice != 0)
		{
			printf("Choix inconnu. Entre un nombre (0-9).\n");
			ui_wait_enter();
		}
	}
	
	parser_thread_stop();
}

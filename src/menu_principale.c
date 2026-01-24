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

void	print_status(void);

static void init_storage(void)
{
	FILE *f;
	
	tm_ensure_logs_dir();
	f = fopen(tm_path_hunt_csv(), "ab");
	if (f)
	{
		csv_ensure_header6(f);
		fclose(f);
	}
}

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

	printf("\n=== Tracker Modulaire ===\n");
	printf("Parser         : %s\n",
		parser_thread_is_running() ? "RUNNING" : "STOPPED");
	printf("Active weapon  : %s\n", weapon[0] ? weapon : "(none)");
	printf("Session offset : %ld data lines\n", offset);
	printf("CSV            : %s\n", tm_path_hunt_csv());

	/* "Intelligent" helpers: show cost/shot and warnings */
	ini_ok = fs_file_exists(tm_path_armes_ini());
	if (!ini_ok)
	{
		printf("Warning        : armes.ini not found (%s)\n", tm_path_armes_ini());
		return ;
	}
	if (!weapon[0])
		return ;
	memset(&db, 0, sizeof(db));
	if (armes_db_load(&db, tm_path_armes_ini()) != 0)
	{
		printf("Warning        : failed to load armes.ini\n");
		return ;
	}
	w = armes_db_find(&db, weapon);
	if (!w)
		printf("Warning        : active weapon not in armes.ini\n");
	else
	{
		cost_shot = arme_cost_shot(w);
		printf("Cost/shot      : %.6f PED\n", cost_shot);
	}
	armes_db_free(&db);
}

static void	print_menu(void)
{
	//ui_clear_viewport();

	printf("####### ###    ## ######## ####### ####### ####### ## #######\n");
	printf("##      ## #   ##    ##    ##   ## ##   ## ##   ## ## ##   ##\n");
	printf("#####   ##  #  ##    ##    ####### ##   ## ####### ## #######\n");
	printf("##      ##   # ##    ##    ## ##   ##   ## ##      ## ##   ##\n");
	printf("####### ##    ###    ##    ##   ## ####### ##      ## ##   ##\n");
	printf("                                                             \n");
	printf("          #  # ##   # # #   # #### #### #### ####            \n");
	printf("          #  # # #  # # #   # #    #  # #    #               \n");
	printf("=====     #  # # ## # # #   # ###  #### #### ###        =====\n");
	printf("          #  # #  # # #  # #  #    # #     # #               \n");
	printf("          #### #   ## #   #   #### #  # #### ####            \n");
	
	printf("\n");
	printf("\n");
	
	printf("\n1) Tracker Chasse (full menu)\n");
	printf("2) Start parser LIVE\n");
	printf("3) Start parser REPLAY\n");
	printf("4) Stop parser\n");
	printf("5) Show stats (from session offset)\n");
	printf("6) Live dashboard (auto-refresh)\n");
	printf("7) Set session offset = current CSV end\n");
	printf("8) Reset session offset = 0\n");
	printf("9) Clear hunt CSV (type YES)\n");
	printf("0) Quit\n");
	printf("Choice: ");
}

static void	menu_show_stats(void)
{
	long		offset;
	t_hunt_stats	s;

	memset(&s, 0, sizeof(s));
	offset = session_load_offset(tm_path_session_offset());
	if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) != 0)
	{
		printf("[ERROR] cannot compute stats (missing CSV?)\n");
		return ;
	}
	tracker_view_print(&s);
}

static void	menu_clear_csv(void)
{
	char	confirm[8];
	FILE	*f;

	printf("Type YES to clear %s: ", tm_path_hunt_csv());
	if (scanf("%7s", confirm) != 1)
	{
		ui_flush_stdin();
		return ;
	}
	ui_flush_stdin();
	if (strcmp(confirm, "YES") != 0)
	{
		printf("Cancelled.\n");
		return ;
	}
	f = fopen(tm_path_hunt_csv(), "w");
	if (!f)
	{
		printf("[ERROR] cannot open CSV for writing\n");
		return ;
	}
	fclose(f);
	printf("CSV cleared.\n");
	/* Optional safety: reset session offset too */
	session_save_offset(tm_path_session_offset(), 0);
	printf("Session offset reset to 0\n");
}

void	menu_principale(void)
{
	init_storage();
	int	choice;
	long	offset;

	if (tm_ensure_logs_dir() != 0)
		printf("[WARN] cannot create logs/\n");
	choice = -1;
	while (choice != 0)
	{
		print_status();
		print_menu();
		if (scanf("%d", &choice) != 1)
		{
			ui_flush_stdin();
			continue;
		}
		ui_flush_stdin();
		if (choice == 1){
			print_status();
			menu_tracker_chasse();
		}
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
			printf("Session offset set to %ld data lines\n", offset);
		}
		else if (choice == 8)
		{
			session_save_offset(tm_path_session_offset(), 0);
			printf("Session offset reset to 0\n");
		}
		else if (choice == 9)
			menu_clear_csv();
	}
	parser_thread_stop();
}

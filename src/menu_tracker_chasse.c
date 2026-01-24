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

#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Weapons helpers                                                            */
/* -------------------------------------------------------------------------- */

static void	print_weapon(const arme_stats *w)
{
	if (!w)
		return ;
	printf("\n--- Active weapon ---\n");
	printf("Name          : %s\n", w->name);
	printf("DPP           : %.4f\n", w->dpp);
	printf("Ammo / shot   : %.6f PED\n", w->ammo_shot);
	printf("Decay / shot  : %.6f PED\n", w->decay_shot);
	printf("Amp decay/shot: %.6f PED\n", w->amp_decay_shot);
	printf("Markup        : %.4f\n", w->markup);
	printf("Cost / shot   : %.6f PED\n", arme_cost_shot(w));
	if (w->notes[0])
		printf("Notes         : %s\n", w->notes);
}

static int	load_db(armes_db *db)
{
	memset(db, 0, sizeof(*db));
	if (!armes_db_load(db, tm_path_armes_ini()))
	{
		printf("Cannot load %s\n", tm_path_armes_ini());
		return (-1);
	}
	return (0);
}

static void	menu_weapon_reload(void)
{
	armes_db	db;

	if (load_db(&db) != 0)
		return ;
	printf("Loaded %zu weapons from %s\n", db.count, tm_path_armes_ini());
	if (db.player_name[0])
		printf("Player: %s\n", db.player_name);
	armes_db_free(&db);
}

static void	menu_weapon_show_active(void)
{
	armes_db		db;
	char			selected[128];
	const arme_stats	*w;

	selected[0] = '\0';
	if (weapon_selected_load(tm_path_weapon_selected(), selected,
			sizeof(selected)) != 0 || !selected[0])
	{
		printf("No active weapon. Use 'Choose active weapon'.\n");
		return ;
	}
	if (load_db(&db) != 0)
		return ;
	w = armes_db_find(&db, selected);
	if (!w)
		printf("Active weapon '%s' not found in %s\n",
			selected, tm_path_armes_ini());
	else
		print_weapon(w);
	armes_db_free(&db);
}

static void	menu_weapon_choose(void)
{
	armes_db	db;
	int		choice;

	if (load_db(&db) != 0)
		return ;
	if (db.count == 0)
	{
		printf("No weapons in %s\n", tm_path_armes_ini());
		armes_db_free(&db);
		return ;
	}
	printf("\n--- Weapons list (%zu) ---\n", db.count);
	for (size_t i = 0; i < db.count; ++i)
		printf("%zu) %s\n", i + 1, db.items[i].name);
	printf("0) Cancel\n");
	printf("Choice: ");
	if (scanf("%d", &choice) != 1)
	{
		ui_flush_stdin();
		armes_db_free(&db);
		return ;
	}
	ui_flush_stdin();
	if (choice <= 0 || (size_t)choice > db.count)
	{
		armes_db_free(&db);
		return ;
	}
	if (weapon_selected_save(tm_path_weapon_selected(),
			db.items[(size_t)choice - 1].name) == 0)
	{
		printf("Active weapon set to: %s\n",
			db.items[(size_t)choice - 1].name);
		print_weapon(&db.items[(size_t)choice - 1]);
	}
	else
		printf("Cannot write %s\n", tm_path_weapon_selected());
	armes_db_free(&db);
}

void	menu_dashboard(void)
{
	long		offset;
	t_hunt_stats	s;

	printf("\nDashboard (press 'q' to quit)\n");
	ui_sleep_ms(250);
	while (!ui_user_wants_quit())
	{
		offset = session_load_offset(tm_path_session_offset());
		if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
		{
			ui_clear_viewport();
			printf("=== Hunt dashboard ===\n");
			printf("Parser: %s\n", parser_thread_is_running() ? "RUNNING" : "STOPPED");
			printf("Session offset: %ld data lines\n\n", offset);
			tracker_view_print(&s);
			printf("\n(press 'q' to quit dashboard)\n");
		}
		else
		{
			ui_clear_viewport();
			printf("No CSV found: %s\n", tm_path_hunt_csv());
			printf("(press 'q' to quit dashboard)\n");
		}
		ui_sleep_ms(800);
	}
}

static void	print_menu(void)
{
	//ui_clear_viewport();
	
	printf("\n=== Tracker Chasse (modulaire) ===\n");
	printf("1) Start parser LIVE\n");
	printf("2) Start parser REPLAY\n");
	printf("3) Stop parser\n");
	printf("4) Show stats (from session offset)\n");
	printf("5) Set session offset = current CSV end\n");
	printf("6) Reload armes.ini (show count)\n");
	printf("7) Choose active weapon\n");
	printf("8) Show active weapon\n");
	printf("9) Live dashboard (auto-refresh)\n");
	printf("0) Back\n");
	printf("Choice: ");
}

void	menu_tracker_chasse(void)
{
	int		choice;
	long	offset;
	t_hunt_stats	s;

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
		if (choice == 1)
			parser_thread_start_live();
		else if (choice == 2)
			parser_thread_start_replay();
		else if (choice == 3)
			parser_thread_stop();
		else if (choice == 4)
		{
			offset = session_load_offset(tm_path_session_offset());
			if (tracker_stats_compute(tm_path_hunt_csv(), offset, &s) == 0)
				tracker_view_print(&s);
			else
				printf("No CSV found: %s\n", tm_path_hunt_csv());
		}
		else if (choice == 5)
		{
			offset = session_count_data_lines(tm_path_hunt_csv());
			session_save_offset(tm_path_session_offset(), offset);
			printf("Session offset set to %ld data lines\n", offset);
		}
		else if (choice == 6)
			menu_weapon_reload();
		else if (choice == 7)
			menu_weapon_choose();
		else if (choice == 8)
			menu_weapon_show_active();
		else if (choice == 9)
			menu_dashboard();
	}
	parser_thread_stop();
}

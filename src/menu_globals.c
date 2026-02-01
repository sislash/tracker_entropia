/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   menu_globals.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login      #+#    #+#                    */
/*   Updated: 2026/01/31 00:00:00 by login      ###   ########.fr             */
/*                                                                            */
/* ************************************************************************** */

#include "menu_globals.h"
#include "menu_principale.h"

#include "core_paths.h"
#include "globals_stats.h"
#include "globals_thread.h"
#include "globals_view.h"
#include "csv.h"
#include "ui_key.h"
#include "ui_utils.h"

#include <stdio.h>
#include <string.h>

static int	read_choice_int(int *out)
{
    int	ret;
    
    if (!out)
        return (0);
    ret = scanf("%d", out);
    ui_flush_stdin();
    return (ret == 1);
}

static void	ensure_globals_csv(void)
{
    FILE	*f;
    
    f = fopen(tm_path_globals_csv(), "ab");
    if (!f)
        return ;
    csv_ensure_header6(f);
    fclose(f);
}

static int	menu_confirm_yes(void)
{
    char	confirm[8];
    
    print_menu_line("Tape YES pour confirmer : ");
    if (scanf("%7s", confirm) != 1)
    {
        ui_flush_stdin();
        printf("Annule.\n");
        return (0);
    }
    ui_flush_stdin();
    if (strcmp(confirm, "YES") != 0)
    {
        printf("Annule.\n");
        return (0);
    }
    return (1);
}

static void	menu_globals_clear_csv(void)
{
    FILE	*f;
    
    print_status_linef("\nATTENTION : tu vas vider le fichier :",
                       "%s", tm_path_globals_csv());
    if (!menu_confirm_yes())
        return ;
    f = fopen(tm_path_globals_csv(), "w");
    if (!f)
    {
        printf("[ERREUR] Impossible d'ouvrir le CSV en ecriture.\n");
        return ;
    }
    fclose(f);
    ensure_globals_csv();
    print_menu_line("OK : CSV globals vide.");
}

static void	menu_globals_dashboard_live(void)
{
    t_globals_stats	s;
    int			c;
    
    printf("\nDASHBOARD GLOBALS LIVE\n");
    printf("Appuie sur 'q' pour quitter.\n");
    ui_sleep_ms(200);
    while (1)
    {
        memset(&s, 0, sizeof(s));
        (void)globals_stats_compute(tm_path_globals_csv(), 0, &s);
        globals_view_print(&s);
        if (ui_key_available())
        {
            c = ui_key_getch();
            if (c == 'q' || c == 'Q' || c == 27)
                break ;
        }
        ui_sleep_ms(250);
    }
}

static void	print_menu(void)
{
    print_hrs();
    print_menu_line("  MENU GLOBALS / GLOBAUX (MOB + CRAFT)");
    print_hrs();
    print_menu_line("Parser (lecture du chat.log)");
    print_menu_line("  1) Demarrer LIVE   (temps reel)");
    print_menu_line("  2) Demarrer REPLAY (relecture)");
    print_menu_line("  3) Arreter le parser");
    print_hr();
    print_menu_line("Dashboard");
    print_menu_line("  4) Dashboard LIVE (auto-refresh)");
    print_hrs();
    print_menu_line("Fichiers");
    print_menu_line("  5) Vider CSV globals (demande confirmation)");
    print_hrs();
    print_menu_line("  0) Retour");
    print_hrs();
    print_menu_line("Choix : ");
}

static void	menu_handle_choice(int choice)
{
    if (choice == 1)
        globals_thread_start_live();
    else if (choice == 2)
        globals_thread_start_replay();
    else if (choice == 3)
    {
        globals_thread_stop();
        printf("\nOK : parser GLOBALS arrete.\n\n");
        ui_wait_enter();
    }
    else if (choice == 4)
        menu_globals_dashboard_live();
    else if (choice == 5)
    {
        menu_globals_clear_csv();
        ui_wait_enter();
    }
    else if (choice != 0)
    {
        printf("Choix inconnu. Entre un nombre (0-5).\n");
        ui_wait_enter();
    }
}

void	menu_globals(void)
{
    int	choice;
    
    if (tm_ensure_logs_dir() != 0)
        printf("[WARN] cannot create logs/\n");
    ensure_globals_csv();
    choice = -1;
    while (choice != 0)
    {
        ui_clear_screen();
        print_status();
        print_menu();
        if (!read_choice_int(&choice))
        {
            printf("Choix invalide. Entre un nombre (0-5).\n");
            continue ;
        }
        menu_handle_choice(choice);
    }
}

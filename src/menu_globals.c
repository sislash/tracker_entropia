/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   menu_globals.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: entropia-tracker                              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25                                #+#    #+#             */
/*   Updated: 2026/01/25                                #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "menu_globals.h"
#include "menu_principale.h"

#include "core_paths.h"
#include "globals_thread.h"
#include "globals_stats.h"
#include "globals_view.h"
#include "ui_utils.h"
#include "ui_key.h"
#include "csv.h"

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

static void	menu_globals_dashboard_live(void)
{
    t_globals_stats	s;
    
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
            int c = ui_key_getch();
            if (c == 'q' || c == 'Q' || c == 27)
                break;
        }
        ui_sleep_ms(250);
    }
}

static void	menu_clear_csv_globals(void)
{
    char	confirm[8];
    FILE	*f;
    
    print_status_linef("\nATTENTION : tu vas vider le fichier :\n%s\n", tm_path_globals_csv());
    print_menu_line("Tape YES pour confirmer : ");
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
    
    /* remets le header */
    f = fopen(tm_path_globals_csv(), "ab");
    if (f)
    {
        csv_ensure_header6(f);
        fclose(f);
    }
    print_menu_line("OK : CSV globals vide.");
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

void	menu_globals(void)
{
    int	choice;
    
    if (tm_ensure_logs_dir() != 0)
        printf("[WARN] cannot create logs/\n");
    
    /* assure que le CSV existe avec header */
    {
        FILE *f = fopen(tm_path_globals_csv(), "ab");
        if (f)
        {
            csv_ensure_header6(f);
            fclose(f);
        }
    }
    
    choice = -1;
    while (choice != 0)
    {
        ui_clear_screen();
        print_status();
        print_menu();
        
        if (!read_choice_int(&choice))
        {
            printf("Choix invalide. Entre un nombre (0-5).\n");
            continue;
        }
        
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
            menu_clear_csv_globals();
            ui_wait_enter();
        }
        else if (choice != 0)
        {
            printf("Choix inconnu. Entre un nombre (0-5).\n");
            ui_wait_enter();
        }
    }
    
    /* sécurité : on ne force pas stop ici, laisse l’utilisateur décider */
}

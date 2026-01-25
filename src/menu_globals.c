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

#include "core_paths.h"
#include "globals_stats.h"
#include "globals_view.h"
#include "ui_utils.h"

#include <string.h>

void	menu_globals_dashboard(void)
{
    t_globals_stats	s;
    
    while (1)
    {
        memset(&s, 0, sizeof(s));
        /* start_line = 0 : on lit tout le CSV globals */
        (void)globals_stats_compute(tm_path_globals_csv(), 0, &s);
        globals_view_print(&s);
        
        /* Quit non-bloquant */
        if (ui_user_wants_quit())
            break ;
        
        ui_sleep_ms(500);
    }
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   globals_view.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ###########       */
/*                                                                            */
/* ************************************************************************** */

#include "globals_view.h"
#include "menu_principale.h"
#include "ui_utils.h"

#include <stdio.h>

/*
 * * Alignement: la barre (| #### |) commence toujours à la même colonne.
 */
#define COL_NAME	28
#define COL_VALUE	12
#define COL_COUNT	8
#define BAR_MAX		30

static void	put_nchar(char c, int n)
{
    while (n > 0)
    {
        putchar(c);
        n--;
    }
}

static int	bar_fill(double v, double vmax)
{
    int	n;
    
    n = 0;
    if (vmax > 0.0)
        n = (int)((v / vmax) * (double)BAR_MAX);
    if (n < 0)
        n = 0;
    if (n > BAR_MAX)
        n = BAR_MAX;
    return (n);
}

static void	print_bar(double v, double vmax)
{
    int	n;
    
    n = bar_fill(v, vmax);
    put_nchar('#', n);
    put_nchar(' ', BAR_MAX - n);
}

static void	print_line(size_t idx, const char *name, double sum,
                       long count, double maxsum)
{
    const char	*safe;
    
    safe = "";
    if (name)
        safe = name;
    printf("%2zu) %-*.*s %*.*f PED | ", idx,
           COL_NAME, COL_NAME, safe, COL_VALUE, 2, sum);
    print_bar(sum, maxsum);
    printf(" | (%*ld)\n", COL_COUNT, count);
}

static double	max_sum(const t_globals_top *arr, size_t n)
{
    double	m;
    size_t	i;
    
    m = 0.0;
    i = 0;
    while (i < n)
    {
        if (arr[i].sum_ped > m)
            m = arr[i].sum_ped;
        i++;
    }
    return (m);
}

static void	print_top_block(const char *title, const t_globals_top *arr,
                            size_t n)
{
    double	mx;
    size_t	i;
    
    printf("%s\n", title);
    print_hr();
    if (!arr || n == 0)
    {
        printf("(aucun)\n");
        return ;
    }
    mx = max_sum(arr, n);
    i = 0;
    while (i < n)
    {
        print_line(i + 1, arr[i].name, arr[i].sum_ped, arr[i].count, mx);
        i++;
    }
}

void	globals_view_print(const t_globals_stats *s)
{
    if (!s)
        return ;
    ui_clear_viewport();
    printf("============================================================\n");
    printf("  DASHBOARD GLOBALS / HOF / ATH (MOB + CRAFT)\n");
    printf("============================================================\n");
    printf("CSV header detecte : %s\n", (s->csv_has_header ? "oui" : "non"));
    printf("Lignes data lues   : %ld\n", s->data_lines_read);
    print_hr();
    printf("RESUME\n");
    print_hr();
    printf("%-22s : %ld (%.2f PED)\n", "Mob events",
           s->mob_events, s->mob_sum_ped);
    printf("%-22s : %ld (%.2f PED)\n", "Craft events",
           s->craft_events, s->craft_sum_ped);
    if (s->rare_events > 0)
    {
        printf("%-22s : %ld (%.4f PED)\n", "Rare events",
               s->rare_events, s->rare_sum_ped);
    }
    print_hr();
    print_top_block("TOP MOBS (somme PED)", s->top_mobs, s->top_mobs_count);
    print_hr();
    print_top_block("TOP CRAFTS (somme PED)", s->top_crafts,
                    s->top_crafts_count);
    if (s->rare_events > 0)
    {
        print_hr();
        print_top_block("TOP RARE ITEMS (somme PED)", s->top_rares,
                        s->top_rares_count);
    }
    printf("============================================================\n");
    printf("q = quitter dashboard\n");
}

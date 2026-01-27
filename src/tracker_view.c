#include "tracker_view.h"
#include "ui_utils.h"
#include "menu_principale.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

/* ************************************************************************** */
/*  Small formatting helpers                                                  */
/* ************************************************************************** */

static void	print_hr(void)
{
	printf("|---------------------------------------------------------------------------|\n");
}

static double	safe_div(double a, double b)
{
	if (b == 0.0)
		return (0.0);
	return (a / b);
}

static double	ratio_pct(double num, double den)
{
	if (den <= 0.0)
		return (0.0);
	return ((num / den) * 100.0);
}

static void	print_ped_pec(const char *label, double ped, int decimals_ped)
{
	long	pec;
	
	pec = (long)llround(ped * 100.0);
	print_status_linef(label, "%.*f PED  (%ld PEC)", decimals_ped, ped, pec);
}

static void	print_ratio_pct_line(const char *label, double num, double den)
{
	if (den <= 0.0)
	{
		print_status_line(label, "n/a");
		return ;
	}
	print_status_linef(label, "%.2f %%", ratio_pct(num, den));
}

static void	print_avg_ped(const char *label, double sum, long n, int decimals)
{
	if (n <= 0)
	{
		print_status_line(label, "n/a");
		return ;
	}
	print_status_linef(label, "%.*f", decimals, safe_div(sum, (double)n));
}

static const char	*yesno(int v)
{
	return (v ? "oui" : "non");
}

/* ************************************************************************** */
/*  Dashboard view                                                            */
/* ************************************************************************** */

void	tracker_view_print(const t_hunt_stats *s)
{
	if (!s)
		return ;
	
	ui_clear_viewport();
	print_status();
	
	/* Header (aligned 75) */
	printf("|===========================================================================|\n");
	print_status_line("DASHBOARD", "CHASSE");
	printf("|===========================================================================|\n");
	
	/* Context */
	print_status_line("CSV header detecte", yesno(s->csv_has_header));
	if (s->has_weapon && s->weapon_name[0])
		print_status_line("Arme active", s->weapon_name);
	if (s->has_weapon && s->player_name[0])
		print_status_line("Joueur (armes.ini)", s->player_name);
	print_hr();
	
	/* Quick summary (what people care first) */
	print_status_line("RESUME", "");
	print_hr();
	
	/* Regroupement Kills / Shots (sans rien perdre) */
	print_status_linef("Kills / Shots", "%ld / %ld", s->kills, s->shots);
	
	print_ped_pec("Loot total", s->loot_ped, 4);
	print_ped_pec("Depense utilisee", s->expense_used, 4);
	print_ped_pec("Net (profit/perte)", s->net_ped, 4);
	
	print_ratio_pct_line("Return", s->loot_ped, s->expense_used);
	print_ratio_pct_line("Profit", s->net_ped, s->expense_used);
	print_hr();
	
	/* Activity */
	print_status_line("ACTIVITE", "");
	print_hr();
	
	print_avg_ped("Shots / kill", (double)s->shots, s->kills, 2);
	
	/* Regroupement Loot events / Expense events */
	if (s->expense_events > 0)
		print_status_linef("Loot/Expense events", "%ld / %ld",
						   s->loot_events, s->expense_events);
	else
		print_status_linef("Loot/Expense events", "%ld / %d",
							s->loot_events, 0);
			
	print_hr();
		
		/* Loot breakdown */
	print_status_line("LOOT (revenus)", "");
	print_hr();
	
	print_avg_ped("Loot / shot", s->loot_ped, s->shots, 6);
	print_avg_ped("Loot / kill", s->loot_ped, s->kills, 4);
	
	print_hr();
	
	/* Sweat */
	print_status_line("SWEAT", "");
	print_hr();
	
	/* Regroupement Extractions / Total */
	print_status_linef("Extractions / Total", "%ld / %ld",
					   s->sweat_events, s->sweat_total);
	
	if (s->sweat_events > 0)
	{
		print_status_linef("Moy / extraction", "%.2f",
						   safe_div((double)s->sweat_total, (double)s->sweat_events));
	}
	else
		print_status_line("Moy / extraction", "n/a");
	
	print_hr();
	
	/* Expenses */
	print_status_line("DEPENSES", "");
	print_hr();
	
	if (s->expense_events > 0)
		print_ped_pec("Depenses (log CSV)", s->expense_ped_logged, 4);
	else
		print_status_line("Depenses (log CSV)", "0 (non detecte)");
	
	if (s->has_weapon && s->shots > 0)
	{
		print_ped_pec("Depenses (modele arme)", s->expense_ped_calc, 4);
		print_ped_pec("Cout par shot", s->cost_shot, 6);
	}
	else
		print_status_line("Modele arme", "n/a (arme absente ou shots=0)");
	
	print_status_line("Source depense",
					  s->expense_used_is_logged ? "log CSV" : "modele arme");
	
	print_hr();
	
	/* Weapon model details (only if usable) */
	if (s->has_weapon && s->shots > 0)
	{
		print_status_line("DETAILS MODELE ARME (cout / shot)", "");
		print_hr();
		
		print_status_linef("Ammo burn / shot", "%.6f PED", s->ammo_shot);
		print_status_linef("Weapon decay / shot", "%.6f PED", s->decay_shot);
		print_status_linef("Amp decay / shot", "%.6f PED", s->amp_decay_shot);
		print_status_linef("Markup (MU)", "x%.3f", s->markup);
		print_status_linef("Cout reel / shot", "%.6f PED", s->cost_shot);
		
		print_hr();
	}
	
	/* Net details */
	print_status_line("RESULTAT", "");
	print_hr();
	
	print_avg_ped("Net / shot", s->net_ped, s->shots, 6);
	print_avg_ped("Net / kill", s->net_ped, s->kills, 4);
	
	print_hr();
	
	/* Top mobs */
	print_status_line("TOP MOBS (kills)", "");
	print_hr();
	
	if (s->top_mobs_count > 0)
	{
		for (size_t i = 0; i < s->top_mobs_count; ++i)
		{
			/* On garde exactement la même info, mais dans une ligne status */
			print_status_linef("",
							   "%2zu) %-28s %ld",
					  i + 1, s->top_mobs[i].name, s->top_mobs[i].kills);
		}
	}
	else
		print_status_line("", "(aucun)");
	
	printf("|===========================================================================|\n");
	printf("\n");
}

#include "tracker_view.h"
#include "ui_utils.h"
#include "menu_principale.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

/* ************************************************************************** */
/*  Small formatting helpers                                                  */
/* ************************************************************************** */

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
	print_hrs();
	print_status_line("DASHBOARD", "CHASSE");
	print_hrs();
	
	/* Context */
	print_status_line(
		"Lecture du fichier",
		s->csv_has_header ? "en-tete detectee (OK)" : "sans en-tete"
	);
	if (s->has_weapon && s->weapon_name[0])
		print_status_line("Arme active", s->weapon_name);
	if (s->has_weapon && s->player_name[0])
		print_status_line("Joueur (armes.ini)", s->player_name);
	print_hrs();
	
	/* Quick summary (what people care first) */
	print_menu_line("RESUME");
	print_hrs();
	
	/* Regroupement Kills / Shots (sans rien perdre) */
	print_status_linef("Kills / Shots", "%ld / %ld", s->kills, s->shots);
	
	/* TT totals (on ne supprime rien) */
	print_ped_pec("Loot total", s->loot_ped, 4);
	print_ped_pec("Depense utilisee", s->expense_used, 4);
	print_ped_pec("Net (profit/perte)", s->net_ped, 4);
	
	print_ratio_pct_line("Return", s->loot_ped, s->expense_used);
	print_ratio_pct_line("Profit", s->net_ped, s->expense_used);
	
	/* MARKUP totals (optionnel, sans enlever le TT) */
	#ifdef TM_STATS_HAS_MARKUP
	print_hrs();
	print_menu_line("MARKUP (optionnel)");
	print_hrs();
	
	print_ped_pec("Loot TT (detail)", s->loot_tt_ped, 4);
	print_ped_pec("Bonus MU (detail)", s->loot_mu_ped, 4);
	print_ped_pec("Loot total (TT+MU)", s->loot_total_mu_ped, 4);
	
	/* Return/Profit avec MU (on ajoute, on ne remplace pas) */
	print_ratio_pct_line("Return (TT+MU)", s->loot_total_mu_ped, s->expense_used);
	print_ratio_pct_line("Profit (TT+MU)",
						 (s->loot_total_mu_ped - s->expense_used), s->expense_used);
	#endif
	
	print_hrs();
	
	/* Activity */
	print_menu_line("ACTIVITE");
	print_hrs();
	
	print_avg_ped("Shots / kill", (double)s->shots, s->kills, 2);
	
	/* Regroupement Loot events / Expense events */
	if (s->expense_events > 0)
		print_status_linef("Loot/Expense events", "%ld / %ld",
						   s->loot_events, s->expense_events);
	else
		print_status_linef("Loot/Expense events", "%ld / %d",
							   s->loot_events, 0);
			
	print_hrs();
		
	/* Loot breakdown */
	print_menu_line("LOOT (revenus)");
	print_hrs();
	
	print_avg_ped("Loot / shot", s->loot_ped, s->shots, 6);
	print_avg_ped("Loot / kill", s->loot_ped, s->kills, 4);
	
	#ifdef TM_STATS_HAS_MARKUP
	/* On ajoute les averages MU sans retirer les TT */
	print_hrs();
	print_menu_line("LOOT (TT+MU)");
	print_hrs();
	print_avg_ped("LootMU / shot", s->loot_total_mu_ped, s->shots, 6);
	print_avg_ped("LootMU / kill", s->loot_total_mu_ped, s->kills, 4);
	#endif
	
	print_hrs();
	
	/* Sweat */
	print_menu_line("SWEAT");
	print_hrs();
	
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
	
	print_hrs();
	
	/* Expenses */
	print_menu_line("DEPENSES");
	print_hrs();
	
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
	
	print_hrs();
	
	/* Weapon model details (only if usable) */
	if (s->has_weapon && s->shots > 0)
	{
		print_status_line("DETAILS MODELE ARME (cout / shot)", "");
		print_hrs();
		
		print_status_linef("Ammo burn / shot", "%.6f PED", s->ammo_shot);
		print_status_linef("Weapon decay / shot", "%.6f PED", s->decay_shot);
		print_status_linef("Amp decay / shot", "%.6f PED", s->amp_decay_shot);
		print_status_linef("Markup (MU)", "x%.3f", s->markup);
		print_status_linef("Cout reel / shot", "%.6f PED", s->cost_shot);
		
		print_hrs();
	}
	
	/* Net details */
	print_menu_line("RESULTAT");
	print_hrs();
	
	print_avg_ped("Net / shot", s->net_ped, s->shots, 6);
	print_avg_ped("Net / kill", s->net_ped, s->kills, 4);
	
	#ifdef TM_STATS_HAS_MARKUP
	/* Net avec MU (sans enlever le TT) */
	print_hrs();
	print_menu_line("RESULTAT (TT+MU)");
	print_hrs();
	print_avg_ped("NetMU / shot", (s->loot_total_mu_ped - s->expense_used), s->shots, 6);
	print_avg_ped("NetMU / kill", (s->loot_total_mu_ped - s->expense_used), s->kills, 4);
	#endif
	
	print_hrs();
	
	/* Top mobs */
	print_menu_line("TOP MOBS (kills)");
	print_hrs();
	
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
	
	/* Footer */
	print_hrs();
}

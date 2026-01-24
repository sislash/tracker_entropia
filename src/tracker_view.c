#include "tracker_view.h"
#include "ui_utils.h"
#include "menu_principale.h"

#include <stdio.h>
#include <math.h>

/* ************************************************************************** */
/*  Small formatting helpers                                                  */
/* ************************************************************************** */

static void	print_hr(void)
{
	printf("------------------------------------------------------------\n");
}

static void	print_ped_pec(const char *label, double ped, int decimals_ped)
{
	long	pec;
	
	pec = (long)llround(ped * 100.0);
	printf("%-22s : %.*f PED  (%ld PEC)\n", label, decimals_ped, ped, pec);
}

static void	print_ratio_pct(const char *label, double num, double den)
{
	if (den <= 0.0)
	{
		printf("%-22s : n/a\n", label);
		return ;
	}
	printf("%-22s : %.2f %%\n", label, (num / den) * 100.0);
}

static void	print_avg_ped(const char *label, double sum, long n, int decimals)
{
	if (n <= 0)
	{
		printf("%-22s : n/a\n", label);
		return ;
	}
	printf("%-22s : %.*f\n", label, decimals, sum / (double)n);
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
	
	printf("\n============================================================\n");
	printf("  DASHBOARD CHASSE\n");
	printf("============================================================\n");
	
	/* Context */
	printf("CSV header detecte       : %s\n", yesno(s->csv_has_header));
	if (s->has_weapon && s->weapon_name[0])
		printf("Arme active              : %s\n", s->weapon_name);
	if (s->has_weapon && s->player_name[0])
		printf("Joueur (armes.ini)       : %s\n", s->player_name);
	print_hr();
	
	/* Quick summary (what people care first) */
	printf("RESUME\n");
	print_hr();
	printf("%-22s : %ld\n", "Kills", s->kills);
	printf("%-22s : %ld\n", "Shots", s->shots);
	print_ped_pec("Loot total", s->loot_ped, 4);
	print_ped_pec("Depense utilisee", s->expense_used, 4);
	print_ped_pec("Net (profit/perte)", s->net_ped, 4);
	print_ratio_pct("Return", s->loot_ped, s->expense_used);
	print_ratio_pct("Profit", s->net_ped, s->expense_used);
	print_hr();
	
	/* Activity */
	printf("ACTIVITE\n");
	print_hr();
	print_avg_ped("Shots / kill", (double)s->shots, s->kills, 2);
	printf("%-22s : %ld\n", "Loot events", s->loot_events);
	if (s->expense_events > 0)
		printf("%-22s : %ld\n", "Expense events", s->expense_events);
	else
		printf("%-22s : 0\n", "Expense events");
	print_hr();
	
	/* Loot breakdown */
	printf("LOOT (revenus)\n");
	print_hr();
	print_avg_ped("Loot / shot", s->loot_ped, s->shots, 6);
	print_avg_ped("Loot / kill", s->loot_ped, s->kills, 4);
	print_hr();
	
	printf("SWEAT\n");
	print_hr();
	printf("%-22s : %ld\n", "Extractions", s->sweat_events);
	printf("%-22s : %ld\n", "Vibrant Sweat total", s->sweat_total);
	if (s->sweat_events > 0)
	{
		printf("%-22s : %.2f\n", "Moy / extraction",
			   (double)s->sweat_total / (double)s->sweat_events);
	}
	print_hr();
	
	/* Expenses */
	printf("DEPENSES\n");
	print_hr();
	if (s->expense_events > 0)
		print_ped_pec("Depenses (log CSV)", s->expense_ped_logged, 4);
	else
		printf("%-22s : %s\n", "Depenses (log CSV)", "0 (non detecte)");
	if (s->has_weapon && s->shots > 0)
	{
		print_ped_pec("Depenses (modele arme)", s->expense_ped_calc, 4);
		print_ped_pec("Cout par shot", s->cost_shot, 6);
	}
	else
		printf("%-22s : %s\n", "Modele arme", "n/a (arme absente ou shots=0)");
	
	printf("%-22s : %s\n", "Source depense",
		   s->expense_used_is_logged ? "log CSV" : "modele arme");
	print_hr();
	
	/* Weapon model details (only if usable) */
	if (s->has_weapon && s->shots > 0)
	{
		printf("DETAILS MODELE ARME (cout / shot)\n");
		print_hr();
		printf("%-22s : %.6f PED\n", "Ammo burn / shot", s->ammo_shot);
		printf("%-22s : %.6f PED\n", "Weapon decay / shot", s->decay_shot);
		printf("%-22s : %.6f PED\n", "Amp decay / shot", s->amp_decay_shot);
		printf("%-22s : x%.3f\n", "Markup (MU)", s->markup);
		printf("%-22s : %.6f PED\n", "Cout reel / shot", s->cost_shot);
		print_hr();
	}
	
	/* Net details */
	printf("RESULTAT\n");
	print_hr();
	print_avg_ped("Net / shot", s->net_ped, s->shots, 6);
	print_avg_ped("Net / kill", s->net_ped, s->kills, 4);
	print_hr();
	
	/* Top mobs */
	printf("TOP MOBS (kills)\n");
	print_hr();
	if (s->top_mobs_count > 0)
	{
		for (size_t i = 0; i < s->top_mobs_count; ++i)
			printf("%2zu) %-28s %ld\n",
				   i + 1, s->top_mobs[i].name, s->top_mobs[i].kills);
	}
	else
		printf("(aucun)\n");
	
	printf("============================================================\n\n");
}

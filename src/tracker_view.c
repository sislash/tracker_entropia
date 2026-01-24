#include "tracker_view.h"
#include "ui_utils.h"
#include "menu_principale.h"

#include <stdio.h>
#include <math.h>

static void	print_ped_pec(const char *label, double ped, int decimals_ped)
{
	long	pec;

	pec = (long)llround(ped * 100.0);
	printf("%-26s %.*f PED  (%ld PEC)\n", label, decimals_ped, ped, pec);
}

static void	print_ratio_pct(const char *label, double num, double den)
{
	if (den <= 0.0)
	{
		printf("%-26s n/a\n", label);
		return ;
	}
	printf("%-26s %.2f %%\n", label, (num / den) * 100.0);
}

static void	print_avg(const char *label, double sum, long n, int decimals)
{
	if (n <= 0)
	{
		printf("%-26s n/a\n", label);
		return ;
	}
	printf("%-26s %.*f\n", label, decimals, sum / (double)n);
}

void	tracker_view_print(const t_hunt_stats *s)
{
	ui_clear_viewport();
	print_status();
	size_t	i;

	if (!s)
		return ;
	printf("\n==================== TRACKER CHASSE ====================\n");
	printf("Header CSV detecte       : %s\n", s->csv_has_header ? "oui" : "non");
	if (s->has_weapon && s->weapon_name[0])
		printf("Arme active              : %s\n", s->weapon_name);
	if (s->has_weapon && s->player_name[0])
		printf("Joueur (armes.ini)       : %s\n", s->player_name);

	printf("\n--- Activite ---\n");
	printf("%-26s %ld\n", "Kills (KILL)", s->kills);
	printf("%-26s %ld\n", "Tirs (SHOT)", s->shots);
	print_avg("Shots / kill", (double)s->shots, s->kills, 2);

	printf("\n--- Loot (revenus) ---\n");
	print_ped_pec("Loot total (brut)", s->loot_ped, 4);
	printf("%-26s %ld\n", "Loot events (lignes)", s->loot_events);
	print_avg("Loot / shot", s->loot_ped, s->shots, 6);
	print_avg("Loot / kill", s->loot_ped, s->kills, 4);

	printf("\n--- Depenses ---\n");
	if (s->expense_events > 0)
	{
		print_ped_pec("Depenses (log CSV)", s->expense_ped_logged, 4);
		printf("%-26s %ld\n", "Expense events (lignes)", s->expense_events);
	}
	else
		printf("%-26s %s\n", "Depenses (log CSV)",
			"0 (aucune ligne expense detectee)");

	if (s->has_weapon && s->shots > 0)
	{
		print_ped_pec("Depenses (calc arme)", s->expense_ped_calc, 4);
		print_ped_pec("Cout arme / shot", s->cost_shot, 6);
		printf("\n--- Modele arme (cout / shot) ---\n");
		printf("Ammo burn / shot        : %.6f PED\n", s->ammo_shot);
		printf("Weapon decay / shot     : %.6f PED\n", s->decay_shot);
		printf("Amp decay / shot        : %.6f PED\n", s->amp_decay_shot);
		printf("Markup (MU applique)    : x%.3f\n", s->markup);
		printf("Cout reel / shot        : %.6f PED\n", s->cost_shot);
	}
	else
		printf("%-26s %s\n", "Depenses (calc arme)",
			"n/a (arme non definie ou shots=0)");

	printf("\n--- Resultat (loot - depense) ---\n");
	print_ped_pec("Depense utilisee", s->expense_used, 4);
	printf("%-26s %s\n", "Source depense utilisee",
		s->expense_used_is_logged ? "log CSV" : "modele arme");
	print_ped_pec("Net (profit/perte)", s->net_ped, 4);
	print_avg("Net / shot", s->net_ped, s->shots, 6);
	print_avg("Net / kill", s->net_ped, s->kills, 4);

	printf("\n--- Ratios (lecture Entropia) ---\n");
	print_ratio_pct("Return (loot/dep)", s->loot_ped, s->expense_used);
	print_ratio_pct("Profit (net/dep)", s->net_ped, s->expense_used);

	printf("\n--- Top mobs (kills) ---\n");
	if (s->top_mobs_count > 0)
	{
		i = 0;
		while (i < s->top_mobs_count)
		{
			printf("%2zu) %-30s %ld\n", i + 1,
				s->top_mobs[i].name, s->top_mobs[i].kills);
			i++;
		}
	}
	else
		printf("(aucun)\n");
	printf("========================================================\n\n");
}

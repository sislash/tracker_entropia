#ifndef TRACKER_STATS_H
# define TRACKER_STATS_H

# include <stddef.h>

# define TM_TOP_MOBS 10

typedef struct s_top_mob
{
	char	name[128];
	long	kills;
} 			t_top_mob;

typedef struct s_hunt_stats
{
	int		csv_has_header;
	long		data_lines_read;

	long		kills;
	long		shots;

	double		loot_ped;
	long		loot_events;

	double		expense_ped_logged;
	long		expense_events;

	double		expense_ped_calc;
	double		expense_used;
	int		expense_used_is_logged;

	double		net_ped;

	int		has_weapon;
	char		weapon_name[128];
	char		player_name[128];
	double		cost_shot;
	double		ammo_shot;
	double		decay_shot;
	double		amp_decay_shot;
	double		markup;

	size_t		mobs_unique;
	size_t		top_mobs_count;
	t_top_mob	top_mobs[TM_TOP_MOBS];
} 			t_hunt_stats;

int	tracker_stats_compute(const char *csv_path, long start_line, t_hunt_stats *out);

#endif

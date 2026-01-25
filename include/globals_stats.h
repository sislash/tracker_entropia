#ifndef GLOBALS_STATS_H
# define GLOBALS_STATS_H

# include <stddef.h>

typedef struct s_globals_top
{
    char	name[128];
    long	count;
    double	sum_ped;
}	t_globals_top;

typedef struct s_globals_stats
{
    int				csv_has_header;
    long			data_lines_read;
    
    long			mob_events;
    long			craft_events;
    long			rare_events;
    
    double			mob_sum_ped;
    double			craft_sum_ped;
    double			rare_sum_ped;
    
    t_globals_top	top_mobs[10];
    size_t			top_mobs_count;
    
    t_globals_top	top_crafts[10];
    size_t			top_crafts_count;
    
    t_globals_top	top_rares[10];
    size_t			top_rares_count;
}	t_globals_stats;

/* start_line: comme chasse, si tu veux ignorer N lignes de data (header non compté) */
int	globals_stats_compute(const char *csv_path, long start_line, t_globals_stats *out);

#endif

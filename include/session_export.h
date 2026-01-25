#ifndef SESSION_EXPORT_H
# define SESSION_EXPORT_H

# include "tracker_stats.h"

int	session_export_stats_csv(const char *out_csv_path,
                             const t_hunt_stats *s, const char *session_start_ts, const char *session_end_ts);

int	session_extract_range_timestamps(const char *hunt_csv_path, long start_offset,
                                     char *out_start, size_t out_start_sz,
                                     char *out_end, size_t out_end_sz);

#endif

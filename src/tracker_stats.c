#include "tracker_stats.h"

#include "core_paths.h"
#include "csv.h"
#include "config_arme.h"
#include "weapon_selected.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------- small utils ------------------------------ */

typedef struct s_kv
{
	char	*key;
	long	count;
} 			t_kv;

static void	stats_zero(t_hunt_stats *s)
{
	memset(s, 0, sizeof(*s));
}

static char	*xstrdup(const char *s)
{
	size_t	len;
	char	*p;

	if (!s)
		return (NULL);
	len = strlen(s) + 1;
	p = (char *)malloc(len);
	if (!p)
		return (NULL);
	memcpy(p, s, len);
	return (p);
}

static int	str_icontains(const char *hay, const char *needle)
{
	size_t	nlen;
	size_t	i;

	if (!hay || !needle || !needle[0])
		return (0);
	nlen = strlen(needle);
	while (*hay)
	{
		i = 0;
		while (i < nlen && hay[i]
			&& tolower((unsigned char)hay[i])
				== tolower((unsigned char)needle[i]))
			i++;
		if (i == nlen)
			return (1);
		hay++;
	}
	return (0);
}

static int	parse_double(const char *s, double *out)
{
	char	buf[128];
	size_t	i;
	size_t	j;
	char	*end;
	double	v;

	if (!s || !out)
		return (0);
	while (isspace((unsigned char)*s))
		s++;
	if (!*s)
		return (0);
	i = 0;
	j = 0;
	while (s[i] && j + 1 < sizeof(buf))
	{
		if (s[i] == ',')
			buf[j++] = '.';
		else
			buf[j++] = s[i];
		i++;
	}
	buf[j] = '\0';
	end = NULL;
	v = strtod(buf, &end);
	if (end == buf)
		return (0);
	*out = v;
	return (1);
}

static int	extract_value_from_raw(const char *raw, double *out)
{
	const char	*vp;

	if (!raw || !out)
		return (0);
	vp = strstr(raw, "Value:");
	if (!vp)
		vp = strstr(raw, "Valeur:");
	if (!vp)
		return (0);
	vp += 6;
	while (*vp && isspace((unsigned char)*vp))
		vp++;
	return (parse_double(vp, out));
}

static void	kv_inc(t_kv **arr, size_t *len, const char *key)
{
	t_kv	*tmp;
	size_t	i;

	if (!key || !key[0])
		key = "(unknown)";
	i = 0;
	while (i < *len)
	{
		if ((*arr)[i].key && strcmp((*arr)[i].key, key) == 0)
		{
			(*arr)[i].count++;
			return ;
		}
		i++;
	}
	tmp = (t_kv *)realloc(*arr, (*len + 1) * sizeof(**arr));
	if (!tmp)
		return ;
	*arr = tmp;
	(*arr)[*len].key = xstrdup(key);
	(*arr)[*len].count = 1;
	(*len)++;
}

static int	kv_cmp_desc(const void *a, const void *b)
{
	const t_kv	*ka;
	const t_kv	*kb;

	ka = (const t_kv *)a;
	kb = (const t_kv *)b;
	if (ka->count < kb->count)
		return (1);
	if (ka->count > kb->count)
		return (-1);
	if (!ka->key)
		return (1);
	if (!kb->key)
		return (-1);
	return (strcmp(ka->key, kb->key));
}

static void	kv_free(t_kv *arr, size_t len)
{
	size_t	i;

	i = 0;
	while (i < len)
	{
		free(arr[i].key);
		i++;
	}
	free(arr);
}

static void	load_weapon_model(t_hunt_stats *out)
{
	armes_db		db;
	char			selected[128];
	const arme_stats	*w;

	out->has_weapon = 0;
	out->weapon_name[0] = '\0';
	out->player_name[0] = '\0';
	selected[0] = '\0';
	if (weapon_selected_load(tm_path_weapon_selected(), selected,
			sizeof(selected)) != 0)
		return ;
	if (!selected[0])
		return ;
	memset(&db, 0, sizeof(db));
	if (!armes_db_load(&db, tm_path_armes_ini()))
		return ;
	w = armes_db_find(&db, selected);
	if (w)
	{
		out->has_weapon = 1;
		snprintf(out->weapon_name, sizeof(out->weapon_name), "%s", w->name);
		if (db.player_name[0])
			snprintf(out->player_name, sizeof(out->player_name), "%s",
				db.player_name);
		out->ammo_shot = w->ammo_shot;
		out->decay_shot = w->decay_shot;
		out->amp_decay_shot = w->amp_decay_shot;
		out->markup = w->markup;
		out->cost_shot = arme_cost_shot(w);
	}
	armes_db_free(&db);
}

int	tracker_stats_compute(const char *csv_path, long start_line, t_hunt_stats *out)
{
	FILE	*f;
	char	buf[8192];
	long	data_idx;
	char	linecpy[8192];
	char	*cols[6];
	t_kv	*mobs;
	size_t	mobs_len;
	int		is_first_line;

	if (!csv_path || !out)
		return (-1);
	stats_zero(out);
	load_weapon_model(out);
	f = fopen(csv_path, "rb");
	if (!f)
		return (-1);
	data_idx = 0;
	mobs = NULL;
	mobs_len = 0;
	is_first_line = 1;
	while (fgets(buf, (int)sizeof(buf), f))
	{
		/* trim \r\n */
		size_t	blen = strlen(buf);
		while (blen && (buf[blen - 1] == '\n' || buf[blen - 1] == '\r'))
			buf[--blen] = '\0';
		if (is_first_line)
		{
			is_first_line = 0;
			if (strstr(buf, "timestamp") && strstr(buf, "type"))
			{
				out->csv_has_header = 1;
				continue;
			}
		}
		if (data_idx < start_line)
		{
			data_idx++;
			continue;
		}
		out->data_lines_read++;
		strncpy(linecpy, buf, sizeof(linecpy) - 1);
		linecpy[sizeof(linecpy) - 1] = '\0';
		csv_split_n(linecpy, cols, 6);

		const char	*type = (cols[1] ? cols[1] : "");
		const char	*name = (cols[2] ? cols[2] : "");
		const char	*qty_s = (cols[3] ? cols[3] : "");
		const char	*val_s = (cols[4] ? cols[4] : "");
		const char	*raw = (cols[5] ? cols[5] : "");

		if (strncmp(type, "KILL", 4) == 0)
		{
			out->kills++;
			kv_inc(&mobs, &mobs_len, name);
			goto next_line;
		}
		if (strcmp(type, "SHOT") == 0)
		{
			long	q = 1;
			if (qty_s && qty_s[0])
				q = atol(qty_s);
			if (q <= 0)
				q = 1;
			out->shots += q;
			goto next_line;
		}

		double	v = 0.0;
		int		has_value = parse_double(val_s, &v);
		if (!has_value && raw && raw[0])
		{
			double	tmpv = 0.0;
			if (extract_value_from_raw(raw, &tmpv))
			{
				v = tmpv;
				has_value = 1;
			}
		}

		int	is_loot = (strncmp(type, "LOOT", 4) == 0)
			|| (strncmp(type, "RECEIVED", 8) == 0);
		int	is_expense = str_icontains(type, "SPEND")
			|| str_icontains(type, "DECAY")
			|| str_icontains(type, "AMMO")
			|| str_icontains(type, "REPAIR");

		if (has_value && (is_loot || (!is_expense && v > 0.0)))
		{
			out->loot_ped += v;
			out->loot_events++;
		}
		else if (has_value && is_expense)
		{
			out->expense_ped_logged += v;
			out->expense_events++;
		}
		next_line:
		data_idx++;
	}
	fclose(f);

	out->mobs_unique = mobs_len;
	if (mobs_len)
	{
		qsort(mobs, mobs_len, sizeof(mobs[0]), kv_cmp_desc);
		out->top_mobs_count = (mobs_len < TM_TOP_MOBS) ? mobs_len : TM_TOP_MOBS;
		for (size_t i = 0; i < out->top_mobs_count; ++i)
		{
			out->top_mobs[i].name[0] = '\0';
			if (mobs[i].key)
				snprintf(out->top_mobs[i].name, sizeof(out->top_mobs[i].name),
					"%s", mobs[i].key);
			out->top_mobs[i].kills = mobs[i].count;
		}
	}

	if (out->has_weapon && out->shots > 0)
		out->expense_ped_calc = (double)out->shots * out->cost_shot;
	out->expense_used_is_logged = (out->expense_events > 0);
	if (out->expense_used_is_logged)
		out->expense_used = out->expense_ped_logged;
	else
		out->expense_used = out->expense_ped_calc;
	out->net_ped = out->loot_ped - out->expense_used;

	kv_free(mobs, mobs_len);
	return (0);
}

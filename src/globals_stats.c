#include "globals_stats.h"

#include "csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------- small utils ------------------------------ */

typedef struct s_kv_sum
{
    char	*key;
    long	count;
    double	sum;
}	t_kv_sum;

static void	stats_zero(t_globals_stats *s)
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

static void	kv_sum_add(t_kv_sum **arr, size_t *len, const char *key, double v)
{
    t_kv_sum	*tmp;
    size_t		i;
    
    if (!key || !key[0])
        key = "(unknown)";
    i = 0;
    while (i < *len)
    {
        if ((*arr)[i].key && strcmp((*arr)[i].key, key) == 0)
        {
            (*arr)[i].count++;
            (*arr)[i].sum += v;
            return ;
        }
        i++;
    }
    tmp = (t_kv_sum *)realloc(*arr, (*len + 1) * sizeof(**arr));
    if (!tmp)
        return ;
    *arr = tmp;
    (*arr)[*len].key = xstrdup(key);
    (*arr)[*len].count = 1;
    (*arr)[*len].sum = v;
    (*len)++;
}

static int	kv_sum_cmp_desc(const void *a, const void *b)
{
    const t_kv_sum	*ka;
    const t_kv_sum	*kb;
    
    ka = (const t_kv_sum *)a;
    kb = (const t_kv_sum *)b;
    if (ka->sum < kb->sum)
        return (1);
    if (ka->sum > kb->sum)
        return (-1);
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

static void	kv_sum_free(t_kv_sum *arr, size_t len)
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

static int	is_mob_type(const char *type)
{
    return (type && strstr(type, "_MOB") != NULL);
}

static int	is_craft_type(const char *type)
{
    return (type && strstr(type, "_CRAFT") != NULL);
}

static int	is_rare_type(const char *type)
{
    return (type && strstr(type, "_RARE") != NULL);
}

static void	fill_top10(t_globals_top out[10], size_t *out_n, t_kv_sum *arr, size_t len)
{
    size_t	n;
    
    *out_n = 0;
    if (!arr || len == 0)
        return ;
    qsort(arr, len, sizeof(*arr), kv_sum_cmp_desc);
    n = len;
    if (n > 10)
        n = 10;
    for (size_t i = 0; i < n; ++i)
    {
        snprintf(out[i].name, sizeof(out[i].name), "%s", arr[i].key ? arr[i].key : "(null)");
        out[i].count = arr[i].count;
        out[i].sum_ped = arr[i].sum;
    }
    *out_n = n;
}

int	globals_stats_compute(const char *csv_path, long start_line, t_globals_stats *out)
{
    FILE	*f;
    char	buf[8192];
    long	data_idx;
    char	linecpy[8192];
    char	*cols[6];
    int		is_first_line;
    
    t_kv_sum	*mobs;
    size_t		mobs_len;
    t_kv_sum	*crafts;
    size_t		crafts_len;
    t_kv_sum	*rares;
    size_t		rares_len;
    
    if (!csv_path || !out)
        return (-1);
    stats_zero(out);
    
    f = fopen(csv_path, "rb");
    if (!f)
        return (-1);
    
    data_idx = 0;
    is_first_line = 1;
    
    mobs = NULL;
    mobs_len = 0;
    crafts = NULL;
    crafts_len = 0;
    rares = NULL;
    rares_len = 0;
    
    while (fgets(buf, (int)sizeof(buf), f))
    {
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
        data_idx++;
        out->data_lines_read++;
        
        strncpy(linecpy, buf, sizeof(linecpy) - 1);
        linecpy[sizeof(linecpy) - 1] = '\0';
        csv_split_n(linecpy, cols, 6);
        
        {
            const char	*type = (cols[1] ? cols[1] : "");
            const char	*name = (cols[2] ? cols[2] : "");
            const char	*val_s = (cols[4] ? cols[4] : "");
            double		v = 0.0;
            
            if (!parse_double(val_s, &v))
                continue;
            
            if (is_mob_type(type))
            {
                out->mob_events++;
                out->mob_sum_ped += v;
                kv_sum_add(&mobs, &mobs_len, name, v);
            }
            else if (is_craft_type(type))
            {
                out->craft_events++;
                out->craft_sum_ped += v;
                kv_sum_add(&crafts, &crafts_len, name, v);
            }
            else if (is_rare_type(type))
            {
                out->rare_events++;
                out->rare_sum_ped += v;
                kv_sum_add(&rares, &rares_len, name, v);
            }
        }
    }
    
    fclose(f);
    
    fill_top10(out->top_mobs, &out->top_mobs_count, mobs, mobs_len);
    fill_top10(out->top_crafts, &out->top_crafts_count, crafts, crafts_len);
    fill_top10(out->top_rares, &out->top_rares_count, rares, rares_len);
    
    kv_sum_free(mobs, mobs_len);
    kv_sum_free(crafts, crafts_len);
    kv_sum_free(rares, rares_len);
    
    return (0);
}

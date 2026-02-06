/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fs_utils.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ###########       */
/*                                                                            */
/* ************************************************************************** */

#include "fs_utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
# include <direct.h>
# include <share.h>
# include <sys/stat.h>
# define TM_MKDIR(path) _mkdir(path)
#else
# include <sys/stat.h>
# include <sys/types.h>
# define TM_MKDIR(path) mkdir(path, 0755)
#endif

static int	mkdir_ok(const char *path)
{
	if (TM_MKDIR(path) != 0 && errno != EEXIST)
		return (-1);
	return (0);
}

static int	cut_to_parent(char *tmp)
{
	size_t	i;
	
	i = strlen(tmp);
	while (i > 0)
	{
		i--;
		if (tmp[i] == '/' || tmp[i] == '\\')
		{
			tmp[i] = '\0';
			return (1);
		}
	}
	return (0);
}

static int	mkdir_recursive(char *tmp)
{
	size_t	i;
	char	saved;
	
	i = 1;
	while (tmp[i])
	{
		if (tmp[i] == '/' || tmp[i] == '\\')
		{
			saved = tmp[i];
			tmp[i] = '\0';
			if (mkdir_ok(tmp) != 0)
				return (-1);
			tmp[i] = saved;
		}
		i++;
	}
	return (mkdir_ok(tmp));
}

#ifdef _WIN32
FILE	*fs_fopen_shared_read(const char *path)
{
	return (_fsopen(path, "rb", _SH_DENYNO));
}
#else
FILE	*fs_fopen_shared_read(const char *path)
{
	return (fopen(path, "rb"));
}
#endif

/*
 * * Crée récursivement les dossiers parents d'un fichier.
 ** Exemple: "logs/hunt_log.csv" => crée "logs/"
 */
int	fs_mkdir_p_for_file(const char *filepath)
{
	char	tmp[1024];
	
	if (!filepath || !*filepath)
		return (-1);
	if (strlen(filepath) >= sizeof(tmp))
		return (-1);
	strcpy(tmp, filepath);
	if (!cut_to_parent(tmp))
		return (0);
	return (mkdir_recursive(tmp));
}

#ifdef _WIN32
static int	is_dir(const char *p)
{
	struct _stat	st;
	
	if (_stat(p, &st) != 0)
		return (0);
	return ((st.st_mode & _S_IFDIR) != 0);
}
#else
static int	is_dir(const char *p)
{
	struct stat	st;
	
	if (stat(p, &st) != 0)
		return (0);
	return (S_ISDIR(st.st_mode));
}
#endif

int	fs_ensure_dir(const char *dir)
{
	if (!dir || !*dir)
		return (-1);
	if (TM_MKDIR(dir) == 0)
		return (0);
	if (errno == EEXIST && is_dir(dir))
		return (0);
	return (-1);
}

int	fs_file_exists(const char *path)
{
	FILE	*f;
	
	if (!path)
		return (0);
	f = fopen(path, "rb");
	if (!f)
		return (0);
	fclose(f);
	return (1);
}

long	fs_file_size(const char *path)
{
	FILE	*f;
	long	sz;
	
	if (!path)
		return (-1);
	f = fopen(path, "rb");
	if (!f)
		return (-1);
	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return (-1);
	}
	sz = ftell(f);
	fclose(f);
	return (sz);
}

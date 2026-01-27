#include "fs_utils.h"

#include <stdio.h>

#include <string.h>
#include <errno.h>

#ifdef _WIN32
# include <direct.h>
# define TM_MKDIR(path) _mkdir(path)
# define TM_PATH_SEP '\\'
#else
# include <sys/stat.h>
# include <sys/types.h>
# define TM_MKDIR(path) mkdir(path, 0755)
# define TM_PATH_SEP '/'
#endif

#ifdef _WIN32
# include <share.h>
FILE *fs_fopen_shared_read(const char *path)
{
	return (_fsopen(path, "rb", _SH_DENYNO));
}
#else
FILE *fs_fopen_shared_read(const char *path)
{
	return (fopen(path, "rb"));
}
#endif

/*
 * * Crée récursivement les dossiers parents d'un fichier.
 ** Exemple: "logs/hunt_log.csv" => crée "logs/"
 */
int fs_mkdir_p_for_file(const char *filepath)
{
	char    tmp[1024];
	size_t  len;
	size_t  i;
	
	if (!filepath || !*filepath)
		return (-1);
	
	len = strlen(filepath);
	if (len >= sizeof(tmp))
		return (-1);
	
	strcpy(tmp, filepath);
	
	/* couper au dernier séparateur */
	for (i = len; i > 0; --i)
	{
		if (tmp[i] == '/' || tmp[i] == '\\')
		{
			tmp[i] = '\0';
			break;
		}
	}
	
	if (i == 0)
		return (0); /* pas de dossier à créer */
		
	/* création récursive */
	for (i = 1; tmp[i]; ++i)
	{
		if (tmp[i] == '/' || tmp[i] == '\\')
		{
			char saved = tmp[i];
			tmp[i] = '\0';
			if (TM_MKDIR(tmp) != 0 && errno != EEXIST)
				return (-1);
			tmp[i] = saved;
		}
	}
	if (TM_MKDIR(tmp) != 0 && errno != EEXIST)
		return (-1);
	
	return (0);
}

#ifdef _WIN32
# include <direct.h>
# include <sys/stat.h>
static int	is_dir(const char *p)
{
	struct _stat st;
	if (_stat(p, &st) != 0)
		return (0);
	return ((st.st_mode & _S_IFDIR) != 0);
}
#else
# include <sys/stat.h>
# include <sys/types.h>
static int	is_dir(const char *p)
{
	struct stat st;
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

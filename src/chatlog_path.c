#include "chatlog_path.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
# include <io.h>
# define ACCESS _access
# define F_OK 0
# define PATH_SEP '\\'
#else
# include <unistd.h>
# define ACCESS access
# define F_OK 0
# define PATH_SEP '/'
#endif

static int	try_path(char *out, size_t outsz, const char *path)
{
	size_t	len;
	
	if (!out || outsz == 0 || !path)
		return (0);
	if (ACCESS(path, F_OK) != 0)
		return (0);
	len = strlen(path);
	if (len + 1 > outsz)
		return (0);
	memcpy(out, path, len + 1);
	return (1);
}

int	chatlog_build_path(char *out, size_t outsz)
{
	const char	*override;
	
	if (!out || outsz == 0)
		return (-1);
	out[0] = '\0';
	
	/* 1) Override possible via env var */
	override = getenv("ENTROPIA_CHATLOG");
	if (override && override[0])
	{
		size_t len = strlen(override);
		if (len + 1 > outsz)
			return (-1);
		memcpy(out, override, len + 1);
		return (0);
	}
	
	#ifdef _WIN32
	{
		const char	*home;
		char		p1[1024];
		char		p2[1024];
		
		home = getenv("USERPROFILE");
		if (!home)
			home = getenv("HOMEPATH");
		if (!home)
			goto fallback;
		
		/* Chemin classique Documents */
		snprintf(p1, sizeof(p1),
				 "%s%cDocuments%cEntropia Universe%cchat.log",
		   home, PATH_SEP, PATH_SEP, PATH_SEP);
		
		if (try_path(out, outsz, p1))
			return (0);
		
		/* Fallback OneDrive (très fréquent) */
		snprintf(p2, sizeof(p2),
				 "%s%cOneDrive%cDocuments%cEntropia Universe%cchat.log",
		   home, PATH_SEP, PATH_SEP, PATH_SEP, PATH_SEP);
		
		if (try_path(out, outsz, p2))
			return (0);
		
		/* Si rien trouvé, on renvoie quand même p1 (utile pour debug) */
		if (strlen(p1) + 1 <= outsz)
		{
			memcpy(out, p1, strlen(p1) + 1);
			return (0);
		}
		return (-1);
	}
	#else
	{
		const char	*home;
		char		p1[1024];
		
		home = getenv("HOME");
		if (!home)
			goto fallback;
		
		snprintf(p1, sizeof(p1),
				 "%s%cDocuments%cEntropia Universe%cchat.log",
		   home, PATH_SEP, PATH_SEP, PATH_SEP);
		
		if (try_path(out, outsz, p1))
			return (0);
		
		/* si pas trouvé, on renvoie quand même p1 (utile debug) */
		if (strlen(p1) + 1 <= outsz)
		{
			memcpy(out, p1, strlen(p1) + 1);
			return (0);
		}
		return (-1);
	}
	#endif
	
	fallback:
	/* 3) last fallback = local file */
	if (outsz >= sizeof("chat.log"))
	{
		memcpy(out, "chat.log", sizeof("chat.log"));
		return (0);
	}
	return (-1);
}

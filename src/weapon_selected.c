#include "weapon_selected.h"

#include <stdio.h>
#include <string.h>

int	weapon_selected_load(const char *path, char *out, size_t outsz)
{
	FILE	*f;

	if (!out || outsz == 0)
		return (-1);
	out[0] = '\0';
	if (!path)
		return (-1);
	f = fopen(path, "rb");
	if (!f)
		return (-1);
	if (!fgets(out, (int)outsz, f))
	{
		fclose(f);
		return (-1);
	}
	fclose(f);
	/* trim newline */
	{
			size_t	len = strlen(out);
		while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
			out[--len] = '\0';
	}
	return (0);
}

int	weapon_selected_save(const char *path, const char *name)
{
	FILE	*f;

	if (!path || !name)
		return (-1);
	f = fopen(path, "wb");
	if (!f)
		return (-1);
	fprintf(f, "%s\n", name);
	fclose(f);
	return (0);
}

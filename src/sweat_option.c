/* sweat_option.c */
#include "sweat_option.h"
#include <stdio.h>
#include <string.h>

int sweat_option_load(const char *path, int *enabled)
{
    FILE    *f;
    char    line[256];
    
    if (!enabled)
        return (-1);
    *enabled = 0;
    if (!path)
        return (-1);
    f = fopen(path, "rb");
    if (!f)
        return (0); /* pas de fichier = OFF par défaut */
        
    while (fgets(line, (int)sizeof(line), f))
    {
        if (strncmp(line, "sweat_tracker=", 14) == 0)
        {
            if (line[14] == '1')
                *enabled = 1;
            break;
        }
    }
    fclose(f);
    return (0);
}

int sweat_option_save(const char *path, int enabled)
{
    FILE    *f;
    
    if (!path)
        return (-1);
    f = fopen(path, "wb");
    if (!f)
        return (-1);
    fprintf(f, "sweat_tracker=%d\n", enabled ? 1 : 0);
    fclose(f);
    return (0);
}

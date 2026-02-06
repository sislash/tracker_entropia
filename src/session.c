/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   session.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "session.h"

#include <stdio.h>
#include <string.h>

long	session_load_offset(const char *path)
{
	FILE	*f;
	long	v;
	
	if (!path)
		return (0);
	f = fopen(path, "rb");
	if (!f)
		return (0);
	v = 0;
	fscanf(f, "%ld", &v);
	fclose(f);
	return (v);
}

int	session_save_offset(const char *path, long offset)
{
	FILE	*f;
	
	if (!path)
		return (-1);
	f = fopen(path, "wb");
	if (!f)
		return (-1);
	fprintf(f, "%ld", offset);
	fclose(f);
	return (0);
}

long	session_count_data_lines(const char *csv_path)
{
	FILE	*f;
	char	buf[4096];
	long	lines;
	int	first;
	
	if (!csv_path)
		return (0);
	f = fopen(csv_path, "rb");
	if (!f)
		return (0);
	lines = 0;
	first = 1;
	while (fgets(buf, (int)sizeof(buf), f))
	{
		if (first)
		{
			first = 0;
			if (strstr(buf, "timestamp") && strstr(buf, "type"))
				continue;
		}
		lines++;
	}
	fclose(f);
	if (lines < 0)
		lines = 0;
	return (lines);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core_paths.c                                        :+:      :+:   :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                 +#+  +:+      +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ###########       */
/*                                                                            */
/* ************************************************************************** */

#include "core_paths.h"
#include "fs_utils.h"

const char	*tm_path_hunt_csv(void)
{
	return (TM_FILE_HUNT_CSV);
}

const char	*tm_path_options_cfg(void)
{
	return (TM_FILE_OPTIONS_CFG);
}

const char	*tm_path_sessions_stats_csv(void)
{
	return (TM_FILE_SESSIONS_STATS_CSV);
}

const char	*tm_path_session_offset(void)
{
	return (TM_FILE_SESSION_OFFSET);
}

const char	*tm_path_weapon_selected(void)
{
	return (TM_FILE_WEAPON_SELECTED);
}

const char	*tm_path_armes_ini(void)
{
	return (TM_FILE_ARMES_INI);
}

/*
 * * Globals / HOF / ATH
 */

const char	*tm_path_globals_csv(void)
{
	return (TM_FILE_GLOBALS_CSV);
}

const char	*tm_path_markup_ini(void)
{
	return (TM_FILE_MARKUP_INI);
}

int	tm_ensure_logs_dir(void)
{
	return (fs_ensure_dir(TM_DIR_LOGS));
}

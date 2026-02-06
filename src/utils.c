/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: you <you@student.42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 00:00:00 by you               #+#    #+#             */
/*   Updated: 2026/02/04 00:00:00 by you              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define _POSIX_C_SOURCE 199309L

#include "utils.h"

#ifndef _WIN32

# include <time.h>

void	ft_sleep_ms(int ms)
{
    struct timespec	ts;
    
    if (ms <= 0)
        return ;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, 0);
}

uint64_t	ft_time_ms(void)
{
    struct timespec	ts;
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL);
}

#else

# include <windows.h>

void	ft_sleep_ms(int ms)
{
    if (ms <= 0)
        return ;
    Sleep((DWORD)ms);
}

uint64_t	ft_time_ms(void)
{
    return ((uint64_t)GetTickCount64());
}

#endif

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ui_utils.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: login <login@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by login             #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by login            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#ifndef _WIN32
# ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 199309L
# endif
#endif


#include "ui_utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
# include <conio.h>
# include <windows.h>
#else
# include <sys/select.h>
# include <time.h>
#endif

/* -------------------------------------------------------------------------- */
/* Timing                                                                     */
/* -------------------------------------------------------------------------- */

void	ui_sleep_ms(unsigned ms)
{
    #ifdef _WIN32
    Sleep((DWORD)ms);
    #else
    struct timespec	ts;
    
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000L);
    nanosleep(&ts, NULL);
    #endif
}

/* -------------------------------------------------------------------------- */
/* Pause                                                                      */
/* -------------------------------------------------------------------------- */

void	ui_wait_enter(void)
{
    printf("Press ENTER to continue...");
    fflush(stdout);
    getchar();
}

/* -------------------------------------------------------------------------- */
/* Cursor / Clear                                                             */
/* -------------------------------------------------------------------------- */

/*
 * Clear the console and reset cursor to top-left.
 * - Windows: WinAPI
 * - Unix: ANSI escape sequences
 */
void	ui_clear_screen(void)
{
    #ifdef _WIN32
    HANDLE						hout;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    DWORD						cell_count;
    DWORD						count;
    COORD						home;
    
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hout == INVALID_HANDLE_VALUE)
        return ;
    if (!GetConsoleScreenBufferInfo(hout, &csbi))
        return ;
    cell_count = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
    count = 0;
    home.X = 0;
    home.Y = 0;
    FillConsoleOutputCharacterA(hout, ' ', cell_count, home, &count);
    FillConsoleOutputAttribute(hout, csbi.wAttributes, cell_count, home, &count);
    SetConsoleCursorPosition(hout, home);
    #else
    fputs("\x1b[2J\x1b[H", stdout);
    fflush(stdout);
    #endif
}

/*
 * Clear only the visible viewport and reset cursor to the viewport top.
 * Useful for dashboards (avoids growing scrollback unnecessarily).
 */
void	ui_clear_viewport(void)
{
    #ifdef _WIN32
    HANDLE						hout;
    CONSOLE_SCREEN_BUFFER_INFO	csbi;
    SHORT						width;
    SHORT						height;
    COORD						top_left;
    DWORD						cells;
    DWORD						count;
    
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hout == INVALID_HANDLE_VALUE)
        return ;
    if (!GetConsoleScreenBufferInfo(hout, &csbi))
        return ;
    width = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    height = (SHORT)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    top_left.X = csbi.srWindow.Left;
    top_left.Y = csbi.srWindow.Top;
    cells = (DWORD)width * (DWORD)height;
    count = 0;
    FillConsoleOutputCharacterA(hout, ' ', cells, top_left, &count);
    FillConsoleOutputAttribute(hout, csbi.wAttributes, cells, top_left, &count);
    SetConsoleCursorPosition(hout, top_left);
    #else
    fputs("\x1b[2J\x1b[H", stdout);
    fflush(stdout);
    #endif
}

void	ui_cursor_home(void)
{
    #ifdef _WIN32
    HANDLE	hout;
    COORD	home;
    
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hout == INVALID_HANDLE_VALUE)
        return ;
    home.X = 0;
    home.Y = 0;
    SetConsoleCursorPosition(hout, home);
    #else
    fputs("\x1b[H", stdout);
    fflush(stdout);
    #endif
}

/* -------------------------------------------------------------------------- */
/* Dashboard helpers                                                          */
/* -------------------------------------------------------------------------- */

void	print_hr(void)
{
    printf("|"
    "---------------------------------------------------------------------------"
    "|\n");
}

void	print_hrs(void)
{
    printf("|"
    "==========================================================================="
    "|\n");
}

void	print_menu_line(const char *text)
{
    char	line[STATUS_WIDTH + 1];
    int		len;
    
    if (!text)
        text = "";
    len = snprintf(line, sizeof(line), "%s", text);
    if (len < 0)
    {
        memset(line, ' ', STATUS_WIDTH);
        line[STATUS_WIDTH] = '\0';
        printf("|%s|\n", line);
        return ;
    }
    if (len < STATUS_WIDTH)
    {
        memset(line + len, ' ', STATUS_WIDTH - len);
        line[STATUS_WIDTH] = '\0';
    }
    else
        line[STATUS_WIDTH] = '\0';
    printf("|%s|\n", line);
}

void	print_status_line(const char *label, const char *value)
{
    char	line[STATUS_WIDTH + 1];
    int		len;
    
    if (!label)
        label = "";
    if (!value)
        value = "";
    len = snprintf(line, sizeof(line), "%s: %s", label, value);
    if (len < 0)
    {
        memset(line, ' ', STATUS_WIDTH);
        line[STATUS_WIDTH] = '\0';
        printf("|%s|\n", line);
        return ;
    }
    if (len < STATUS_WIDTH)
    {
        memset(line + len, ' ', STATUS_WIDTH - len);
        line[STATUS_WIDTH] = '\0';
    }
    else
        line[STATUS_WIDTH] = '\0';
    printf("|%s|\n", line);
}

void	print_status_linef(const char *label, const char *fmt, ...)
{
    char	value[256];
    va_list	ap;
    
    if (!fmt)
        fmt = "";
    va_start(ap, fmt);
    vsnprintf(value, sizeof(value), fmt, ap);
    va_end(ap);
    print_status_line(label, value);
}

/* -------------------------------------------------------------------------- */
/* Input                                                                      */
/* -------------------------------------------------------------------------- */

int	ui_user_wants_quit(void)
{
    #ifdef _WIN32
    int	c;
    
    if (_kbhit())
    {
        c = _getch();
        if (c == 'q' || c == 'Q')
            return (1);
    }
    return (0);
    #else
    struct timeval	tv;
    fd_set			fds;
    int				c;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    if (select(1, &fds, NULL, NULL, &tv) > 0)
    {
        c = getchar();
        if (c == 'q' || c == 'Q')
            return (1);
    }
    return (0);
    #endif
}

void	ui_flush_stdin(void)
{
    int	c;
    
    c = getchar();
    while (c != '\n' && c != EOF)
        c = getchar();
}

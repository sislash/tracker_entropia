#include "ui_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>   /* _kbhit, _getch */

#else
#include <unistd.h>     /* usleep */
#include <time.h>
#include <sys/select.h> /* select */
#endif

/* -------------------------------------------------------------------------- */
/* Timing                                                                     */
/* -------------------------------------------------------------------------- */

void ui_sleep_ms(unsigned ms)
{
    #ifdef _WIN32
    Sleep((DWORD)ms);
    #else
    struct timespec ts;
    
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
    #endif
}

/* -------------------------------------------------------------------------- */
/* Pause                                                                      */
/* -------------------------------------------------------------------------- */

void ui_wait_enter(void)
{
    printf("Press ENTER to continue...");
    fflush(stdout);
    getchar();
}

/* -------------------------------------------------------------------------- */
/* Cursor / Clear                                                              */
/* -------------------------------------------------------------------------- */

/*
 * Efface complètement la console (buffer) et remet le curseur en haut à gauche.
 * - Windows: WinAPI (fiable sur CMD/PowerShell console)
 * - Linux/mac: ANSI
 */
void ui_clear_screen(void)
{
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    
    DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
    DWORD count = 0;
    COORD home = { 0, 0 };
    
    FillConsoleOutputCharacterA(hOut, ' ', cellCount, home, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, home, &count);
    SetConsoleCursorPosition(hOut, home);
    #else
    fputs("\x1b[2J\x1b[H", stdout);
    fflush(stdout);
    #endif
}

/*
 * Efface uniquement la zone visible (viewport/fenêtre) et remet le curseur en haut
 * de la fenêtre.
 * Utile pour les dashboards: évite de "manger" l'historique plus que nécessaire.
 */
void ui_clear_viewport(void)
{
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    
    SHORT width  = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    SHORT height = (SHORT)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    
    COORD topLeft = { csbi.srWindow.Left, csbi.srWindow.Top };
    DWORD cells = (DWORD)width * (DWORD)height;
    DWORD count = 0;
    
    FillConsoleOutputCharacterA(hOut, ' ', cells, topLeft, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, topLeft, &count);
    SetConsoleCursorPosition(hOut, topLeft);
    #else
    /* Sur ANSI, on fait un clear "classique" de l'écran visible */
    fputs("\x1b[2J\x1b[H", stdout);
    fflush(stdout);
    #endif
}

/* Place le curseur en haut à gauche */
void ui_cursor_home(void)
{
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    
    COORD home = { 0, 0 };
    SetConsoleCursorPosition(hOut, home);
    #else
    fputs("\x1b[H", stdout);
    fflush(stdout);
    #endif
}

/*
 * Cette fonction :
 * tronque si trop long
 * remplit avec des espaces si trop court
 * garantit alignement parfait   
*/
void print_hr(void)
{
    printf("|---------------------------------------------------------------------------|\n");
}

void print_hrs(void)
{
    printf("|===========================================================================|\n");
}

void print_menu_line(const char *text)
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

void print_status_line(const char *label, const char *value)
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

void print_status_linef(const char *label, const char *fmt, ...)
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
/* Input                                                                        */
/* -------------------------------------------------------------------------- */

/*
 * Détection non-bloquante de 'q'/'Q'
 * Retourne 1 si pressé, sinon 0.
 */
int ui_user_wants_quit(void)
{
    #ifdef _WIN32
    if (_kbhit())
    {
        int c = _getch();
        if (c == 'q' || c == 'Q')
            return 1;
    }
    return 0;
    #else
    struct timeval tv;
    fd_set fds;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    FD_ZERO(&fds);
    FD_SET(0, &fds); /* stdin */
    
    if (select(1, &fds, NULL, NULL, &tv) > 0)
    {
        int c = getchar();
        if (c == 'q' || c == 'Q')
            return 1;
    }
    return 0;
    #endif
}

void	ui_flush_stdin(void)
{
	int	c;

	c = getchar();
	while (c != '\n' && c != EOF)
		c = getchar();
}

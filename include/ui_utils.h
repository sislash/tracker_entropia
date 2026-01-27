#ifndef UI_UTILS_H
#define UI_UTILS_H

# include <stdarg.h>

# define STATUS_WIDTH 75

void ui_sleep_ms(unsigned ms);
void ui_wait_enter(void);
void ui_clear_screen(void);
void ui_clear_viewport(void);
void print_status_line(const char *label, const char *value);
void print_status_linef(const char *label, const char *fmt, ...);
void ui_cursor_home(void);
int  ui_user_wants_quit(void);
void ui_flush_stdin(void);

#endif

#ifndef UI_UTILS_H
#define UI_UTILS_H

void ui_sleep_ms(unsigned ms);
void ui_clear_screen(void);
void ui_clear_viewport(void);
void ui_cursor_home(void);
int  ui_user_wants_quit(void);
void ui_flush_stdin(void);

#endif

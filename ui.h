#ifndef UI_H
#define UI_H

#include <ncurses.h>


void ui_init(char *room_name);

char* ui_get_message(void);

void ui_render_chat(char *message, char *user);

void ui_shutdown(void);

#endif


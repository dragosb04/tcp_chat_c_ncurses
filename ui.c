#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>


#define CHAT_WIDTH_RATIO 0.75f   // 3/4 of screen
#define TEXTBOX_HEIGHT 3
#define TOP_MARGIN 1
#define SIDE_MARGIN 1
#define TEXTBOX_OFFSET 4

#define COLOR_ORANGE 8
#define COLOR_GRAY 9

#define MESSAGE_MAX_LEN 128

WINDOW *chat;
WINDOW *users;
WINDOW *textbox;

int message_count = 0;

int width_chat  ;
int height_chat ;

int width_users  ;
int height_users ;

int width_text  ;
int height_text ;


int startx_chat ;
int starty_chat;

int startx_users;
int starty_users ;

int startx_text ;
int starty_text;


WINDOW *create_newwin(int height, int width, int starty, int startx)
{	WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0 , 0);		/* 0, 0 gives default characters
                     * for the vertical and horizontal
                     * lines			*/
    wrefresh(local_win);		/* Show that box 		*/

    return local_win;
}

void ui_init(char *room_name) {


    char participants_text[]="Chat participants";
    char type_text[]="Type a message: ";

    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    width_chat  = (int)(CHAT_WIDTH_RATIO * COLS);
    height_chat = LINES - (TEXTBOX_HEIGHT + TOP_MARGIN + (TEXTBOX_OFFSET - TEXTBOX_HEIGHT));

    width_users  = COLS - width_chat - SIDE_MARGIN;
    height_users = LINES - (TOP_MARGIN + 1);

    width_text  = width_chat;
    height_text = TEXTBOX_HEIGHT;

    startx_chat = SIDE_MARGIN;
    starty_chat = TOP_MARGIN;

    startx_users = startx_chat + width_chat;
    starty_users = TOP_MARGIN;

    startx_text = SIDE_MARGIN;
    starty_text = LINES - TEXTBOX_OFFSET;

    init_color(COLOR_ORANGE, 1000, 647, 0);
    init_pair(1, COLOR_ORANGE, COLOR_BLACK);
    attron(COLOR_PAIR(1) | A_BOLD | A_UNDERLINE);
    mvprintw(0,startx_chat,"F1 to exit");
    mvprintw(0,(COLS-strlen(room_name))/2,"%s",room_name);
    refresh();
    attroff(COLOR_PAIR(1) | A_BOLD | A_UNDERLINE);

    chat = create_newwin(height_chat, width_chat, starty_chat, startx_chat);
    users = create_newwin(height_users, width_users, starty_users, startx_users);
    textbox = create_newwin(height_text, width_text, starty_text, startx_text);

    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    attron(COLOR_PAIR(2) | A_UNDERLINE | A_BOLD);
    mvprintw(starty_users+1, startx_users+6,"%s", participants_text);
    attroff(COLOR_PAIR(2) | A_UNDERLINE | A_BOLD);
    refresh();

    init_color(COLOR_GRAY, 500, 500, 500);
    init_pair(3, COLOR_GRAY, COLOR_BLACK);
    attron(COLOR_PAIR(3) | A_DIM | A_INVIS);
    mvwprintw(textbox, 1, 1,"%s", type_text);
    attroff(COLOR_PAIR(3) | A_DIM | A_INVIS);
    wrefresh(textbox);

}

char* ui_get_message() {
    int message_pos = 0;
    char type_text[]="Type a message: ";

    char* message=malloc(sizeof(char)*MESSAGE_MAX_LEN);
    int ch=getch();
    while (ch!=KEY_F(1)) {


        if (ch == '\n' || ch == KEY_ENTER) {
            if (message_pos > 0) {

                // Clear the input area
                int input_width = width_text - 2 - strlen(type_text);

                mvwprintw(textbox, 1, 1 + strlen(type_text),"%*s", input_width, "");
                mvwprintw(textbox, 1, 1, "%s", type_text); // Redraw prompt
                wrefresh(textbox);
            }
            return message;
        }  else if ((ch == KEY_BACKSPACE || ch == 127) && message_pos > 0) {
            message_pos--;
            message[message_pos] = '\0';

            // Update display
            mvwprintw(textbox, 1, 1 + strlen(type_text) + message_pos, " ");
            wrefresh(textbox);
            move(starty_text + 1, startx_text + 1 + strlen(type_text) + message_pos);
        }
        // Handle regular characters
        else if (ch >= 32 && ch <= 126 && message_pos < MESSAGE_MAX_LEN - 1) {
            message[message_pos] = ch;
            message_pos++;
            message[message_pos] = '\0';

            mvwaddch(textbox, 1, 1 + strlen(type_text) + message_pos - 1, ch);
            wrefresh(textbox);
            move(starty_text + 1, startx_text + 1 + strlen(type_text) + message_pos);
        }
        ch=getch();
    }
    return NULL;

}

void ui_render_chat(char* message,char* user) {
    mvwprintw(chat, message_count + 1, 1, "[%s]: %s",user, message);
    wrefresh(chat);
    message_count++;
}

void ui_shutdown() {
    endwin();
}

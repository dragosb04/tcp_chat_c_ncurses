#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ncurses.h>

#define BUFFER_SIZE 1024

int sockfd;

// Ferestre NCURSES
WINDOW *win_messages;
WINDOW *win_input;

pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;


void handle_input(WINDOW *win, char *buffer, int *pos) {
    int ch = wgetch(win);
    if (ch != ERR) {
        if (ch == 127 || ch == KEY_BACKSPACE) { // Backspace
            if (*pos > 0) {
                (*pos)--;
                buffer[*pos] = '\0';
                mvwdelch(win, 1, 1 + *pos); // Șterge vizual
            }
        } else if (ch == '\n') {
            // Enter detectat - procesare externă
        } else if (*pos < BUFFER_SIZE - 1) {
            buffer[*pos] = ch;
            (*pos)++;
            waddch(win, ch);
        }
    }
}

void ui_add_message(const char *msg) {
    pthread_mutex_lock(&ui_mutex);

    // Adaugă mesaj și face scroll
    wprintw(win_messages, "%s\n", msg);
    wrefresh(win_messages);

    pthread_mutex_unlock(&ui_mutex);
}

// Thread pentru recepția mesajelor
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            ui_add_message("\n[Disconnected from server]\n");
            close(sockfd);
            endwin();
            exit(0);
        }

        buffer[bytes] = '\0';
        ui_add_message(buffer);
    }

    return NULL;
}

int connect_to_server(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        return -1;
    }
    return fd;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./client [name]\n");
        exit(1);
    }

    char *name = argv[1];

    struct sockaddr_in server_addr;
    char input_buffer[BUFFER_SIZE];

    // Creează socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Setări server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.01");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    // Trimite numele
    send(sockfd, name, strlen(name), 0);

    // ===============================
    //  INITIALIZARE NCURSES
    // ===============================
    initscr();
    noecho();
    cbreak();

    int height = LINES;
    int width = COLS;

    win_messages = newwin(height - 3, width, 0, 0);
    win_input = newwin(3, width, height - 3, 0);

    scrollok(win_messages, TRUE);
    scrollok(win_input, TRUE);

    box(win_input, 0, 0);

    wrefresh(win_messages);
    wrefresh(win_input);

    // Thread pentru recepție
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);

    // ===============================
    //  LOOP INPUT UTILIZATOR
    // ===============================
while (1) {
    werase(win_input);
    box(win_input, 0, 0);

    echo(); // vezi ce tastezi
    mvwgetnstr(win_input, 1, 1, input_buffer, BUFFER_SIZE - 1);
    noecho(); // dezactivăm pentru restul programului

    send(sockfd, input_buffer, strlen(input_buffer), 0);

    werase(win_input);
    box(win_input, 0, 0);
    wrefresh(win_input);
}


    endwin();
    close(sockfd);
    return 0;
}

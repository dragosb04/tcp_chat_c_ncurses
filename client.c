#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "ui.h"

#define BUFFER_SIZE 1024

int sockfd;

pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;

void *receive_messages(void *arg) {
    static char stream_buf[4096];
    int used = 0;

    while (1) {
        int bytes = recv(sockfd, stream_buf + used,
                          sizeof(stream_buf) - used - 1, 0);
        if (bytes <= 0) break;

        used += bytes;
        stream_buf[used] = '\0';

        char *line_start = stream_buf;
        char *newline;

        while ((newline = strchr(line_start, '\n')) != NULL) {
            *newline = '\0';          // izolează o linie completă
            char line[BUFFER_SIZE];
            strncpy(line, line_start, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';

			// IGNORĂ LINIILE GOALE
    		if (line[0] == '\0') {
        		line_start = newline + 1;
        		continue;
    		}

            // ==== PROCESARE MESAJ ====
            if (strncmp(line, "USERS ", 6) == 0) {
                char *users_list = line + 6;

                char *names[50];
                int count = 0;

                char *token = strtok(users_list, ",");
                while (token && count < 50) {
                    names[count++] = strdup(token);
                    token = strtok(NULL, ",");
                }

                pthread_mutex_lock(&ui_mutex);
                ui_update_users(names, count);
                pthread_mutex_unlock(&ui_mutex);

                for (int i = 0; i < count; i++)
                    free(names[i]);
            } else {
                // Mesaj normal
                char *msg_start = strchr(line, ']');
                if (msg_start && msg_start[1] == ':' && msg_start[2] == ' ') {
                    char *name_start = strchr(line, '[');
                    if (name_start) {
                        char user_name[50];
                        strncpy(user_name, name_start + 1,
                                msg_start - name_start - 1);
                        user_name[msg_start - name_start - 1] = '\0';

                        char *message_text = msg_start + 3;
                        ui_render_chat(message_text, user_name);
                    }
                } else {
                    ui_render_chat(line, "SERVER");
                }
            }

            line_start = newline + 1;
        }

        // mută ce a rămas incomplet la începutul bufferului
        used = strlen(line_start);
        memmove(stream_buf, line_start, used);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Utilizare: ./client [nume]\n");
        exit(1);
    }

    char *name = argv[1];

    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    // Setări server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    // Trimite numele
    send(sockfd, name, strlen(name), 0);

    // ===============================
    //  INITIALIZARE NCURSES CU UI.H
    // ===============================
    ui_init("Camera de Chat"); // Foloseste functia din ui.h

    // Thread pentru recepție
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);

    // ===============================
    //  LOOP INPUT UTILIZATOR CU UI.H
    // ===============================
    char *input_buffer;
    while (1) {
        // ui_get_message returneaza un string alocat dinamic sau NULL la iesire (ex: F1)
        input_buffer = ui_get_message();

        if (input_buffer == NULL) {
            // Semnal de iesire
            break;
        }

        // Trimite mesajul la server
        send(sockfd, input_buffer, strlen(input_buffer), 0);
        free(input_buffer);
    }

    // ===============================
    //  OPRIRE NCURSES CU UI.H
    // ===============================
    ui_shutdown(); // Foloseste functia din ui.h
    close(sockfd);
    return 0;
}
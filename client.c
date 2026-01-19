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
    char peek_buffer[BUFFER_SIZE];
    char user_name[50];
    char message_text[BUFFER_SIZE];

    while (1) {
        // 1. Primul "recv" (cu MSG_PEEK): Vedem ce a trimis serverul
        int total_bytes = recv(sockfd, peek_buffer, sizeof(peek_buffer) - 1, MSG_PEEK);
        if (total_bytes <= 0) break;
        peek_buffer[total_bytes] = '\0';

        // Cautam separatorul ':' folosit de server
        char *separator = strchr(peek_buffer, ':');

        if (separator != NULL) {
            int name_len_in_buffer = (separator - peek_buffer);

            // 2. Al doilea "recv" (REAL): Extragem numele si separatorul ": "
            // Acum datele sunt scoase definitiv din buffer-ul sistemului
            int n_bytes = recv(sockfd, user_name, name_len_in_buffer + 2, 0);
            user_name[n_bytes] = '\0';

            // --- LOGICA DE CURATARE PENTRU A ELIMINA [[ ]]: ---
            char *clean_name = user_name;

            user_name[name_len_in_buffer] = '\0';

            if (clean_name[0] == '[') clean_name++;

            int len = strlen(clean_name);
            if (len > 0 && clean_name[len - 1] == ']') {
                clean_name[len - 1] = '\0';
            }

            // 3. Al treilea apel recv (pentru corpul mesajului)
            int remaining = total_bytes - (name_len_in_buffer + 2);
            int m_bytes = recv(sockfd, message_text, remaining, 0);
            message_text[m_bytes] = '\0';

            ui_render_chat(message_text, clean_name);

        } else {
            // Daca nu exista ':' (mesaj simplu), îl citim complet dintr-un singur recv
            int b = recv(sockfd, message_text, total_bytes, 0);
            message_text[b] = '\0';
            ui_render_chat(message_text, "SERVER");
        }
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

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    send(sockfd, name, strlen(name), 0);

    //  INITIALIZARE NCURSES CU UI.H
    ui_init("Camera de Chat"); // Foloseste functia din ui.h

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);

    //  LOOP INPUT UTILIZATOR CU UI.H

    char *input_buffer;
    while (1) {
        input_buffer = ui_get_message(); 

        if (input_buffer == NULL) {
            break;
        }

        send(sockfd, input_buffer, strlen(input_buffer), 0);
        free(input_buffer);
    }

    //  OPRIRE NCURSES CU UI.H
    ui_shutdown();
    close(sockfd);
    return 0;
}

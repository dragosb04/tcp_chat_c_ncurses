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
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            // Afiseaza mesajul de deconectare
            ui_render_chat("[Disconnected from server]", "SYSTEM");
            close(sockfd);
            ui_shutdown(); // Foloseste functia din ui.h
            exit(0);
        }

        buffer[bytes] = '\0';
        
        // Logica de parsare a mesajului primit de la server
        // Mesajul are formatul general: [NUME]: TEXT
        
        char *message_text = strchr(buffer, ':');
        
        if (message_text != NULL) {
            // Extrage numele utilizatorului/server-ului
            char user_name[50] = {0};
            int name_len = message_text - buffer;
            
            // Verifica daca mesajul incepe cu '[' si contine ']:' (format standard)
            if (buffer[0] == '[' && buffer[name_len - 1] == ']') {
                // Copiaza numele (fara parantezele [ si ])
                strncpy(user_name, buffer + 1, name_len - 2);
                user_name[name_len - 2] = '\0';
                
                // Textul mesajului incepe dupa ': '
                message_text += 2; 

                // Afiseaza mesajul folosind functia din ui.h
                ui_render_chat(message_text, user_name);

            } else {
                // Mesaj fara format standard (il tratam ca pe un mesaj de la server)
                ui_render_chat(buffer, "SERVER"); 
            }
            
        } else {
            // Mesaj care nu contine ':', cel mai probabil un mesaj simplu de la server
            ui_render_chat(buffer, "SERVER");
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
        free(input_buffer); // Elibereaza memoria alocata de ui_get_message
    }

    // ===============================
    //  OPRIRE NCURSES CU UI.H
    // ===============================
    ui_shutdown(); // Foloseste functia din ui.h
    close(sockfd);
    return 0;
}

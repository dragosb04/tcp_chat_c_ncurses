#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

typedef struct {
    int sockfd;
    char name[50];
} client_t;

client_t clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_to_client(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

void send_user_list() {
    char buffer[BUFFER_SIZE] = "USERS ";

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != 0) {
            strcat(buffer, clients[i].name);
            strcat(buffer, ",");
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    int len = strlen(buffer);
	if ( len > 6 ){
    	if (buffer[len - 1] == ','){
			buffer[len - 1] = '\0';
		}
	}

	strcat(buffer, "\n");

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].sockfd != 0)
            send(clients[i].sockfd, buffer, strlen(buffer), 0);
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(char *msg, int sender_fd) {
    char message_out[BUFFER_SIZE + 100];

    if (strncmp(msg, "/pm ", 4) == 0) {
        char *receiver = strtok(msg + 4, " ");
        char *pm_text = strtok(NULL, "");

        if (!receiver || !pm_text)
            return;

        int receiver_fd = -1;
        char sender_name[50];

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sockfd == sender_fd) {
                strcpy(sender_name, clients[i].name);
            }
            if (strcmp(clients[i].name, receiver) == 0) {
                receiver_fd = clients[i].sockfd;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (receiver_fd != -1) {
            snprintf(message_out, sizeof(message_out), "[PM %s to %s]: %s\n", sender_name, receiver, pm_text);

            send_to_client(receiver_fd, message_out);
            send_to_client(sender_fd, message_out);
        }
        return;
    }

    char sender_name[50];
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].sockfd == sender_fd)
            strcpy(sender_name, clients[i].name);
    pthread_mutex_unlock(&clients_mutex);

    if (sender_fd != -1)
        snprintf(message_out, sizeof(message_out), "[%s]: %s\n", sender_name, msg);
    else
        snprintf(message_out, sizeof(message_out), "%s\n", msg);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != 0) {
            send_to_client(clients[i].sockfd, message_out);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void client_disconnected(int index) {
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "[Server]: %s s-a deconectat.", clients[index].name);
    broadcast_message(msg, -1);
    printf("%s s-a deconectat.\n", clients[index].name);
	send_user_list();
    close(clients[index].sockfd);
    clients[index].sockfd = 0;
    clients[index].name[0] = '\0';
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int index = -1;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].sockfd == client_fd)
            index = i;
    pthread_mutex_unlock(&clients_mutex);

    if (index == -1) return NULL;

    while (1) {
        int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            client_disconnected(index);
            break;
        }

        buffer[bytes] = '\0';
        broadcast_message(buffer, client_fd);
    }

    return NULL;
}

int main() {
    int server_fd, new_client;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = 0;
        clients[i].name[0] = '\0';
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    printf("Serverul ruleaza pe portul %d...\n", PORT);

    while (1) {
        new_client = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        char name[50];
        int bytes = recv(new_client, name, sizeof(name) - 1, 0);
        if (bytes <= 0) {
            close(new_client);
            continue;
        }
        name[bytes] = '\0';

        pthread_mutex_lock(&clients_mutex);
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++)
            if (clients[i].sockfd == 0) {
                clients[i].sockfd = new_client;
                strcpy(clients[i].name, name);
                slot = i;
                break;
            }
        pthread_mutex_unlock(&clients_mutex);

        if (slot == -1) {
            close(new_client);
            continue;
        }

        char connect_msg[BUFFER_SIZE];
        snprintf(connect_msg, sizeof(connect_msg),"[Server]: %s s-a conectat!\n", name);
        printf("%s s-a conectat.\n", name);
        broadcast_message(connect_msg, -1);

		send_user_list();


        pthread_t tid;
        int *client_fd_ptr = malloc(sizeof(int));
        *client_fd_ptr = new_client;
        pthread_create(&tid, NULL, handle_client, client_fd_ptr);
        pthread_detach(tid);
    }

	close(server_fd);
    return 0;
}

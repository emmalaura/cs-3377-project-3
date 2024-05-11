#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "msg.h"

#define DATABASE_FILE "database.txt"
#define MAX_CLIENTS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    // Open socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to port
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    // Accept connections and create handler threads
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (*client_fd < 0) {
            perror("accept");
            free(client_fd);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)client_fd) != 0) {
            perror("pthread_create");
            free(client_fd);
            continue;
        }
        pthread_detach(tid);
    }

    // Close socket
    close(listen_fd);

    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    struct msg request;
    ssize_t bytes_received;

    while (1) {
        // Receive client request
        bytes_received = recv(client_fd, &request, sizeof(request), 0);
        if (bytes_received < 0) {
            perror("recv");
            close(client_fd);
            pthread_exit(NULL);
        } else if (bytes_received == 0) {
            close(client_fd);
            pthread_exit(NULL);
        }

        // Handle client request
        if (request.type == PUT) {
            // Write record to database file
            FILE *database = fopen(DATABASE_FILE, "a");
            if (database == NULL) {
                perror("fopen");
                close(client_fd);
                pthread_exit(NULL);
            }
            pthread_mutex_lock(&mutex);
            fwrite(&request.rd, sizeof(request.rd), 1, database);
            pthread_mutex_unlock(&mutex);
            fclose(database);

            // View the contents of the database file
            FILE *viewDatabase = fopen(DATABASE_FILE, "r");
            if (viewDatabase == NULL) {
                perror("fopen");
                close(client_fd);
                pthread_exit(NULL);
            }
            printf("Contents of the database file after PUT:\n");
            struct record temp_record;
            while (fread(&temp_record, sizeof(temp_record), 1, viewDatabase) == 1) {
                printf("Name: %s, ID: %d\n", temp_record.name, temp_record.id);
            }
            fclose(viewDatabase);

            // Send success response
            struct msg response = {SUCCESS};
            if (send(client_fd, &response, sizeof(response), 0) < 0) {
                perror("send");
            }
        } else if (request.type == GET) {
            // Search for record in database file
            FILE *database = fopen(DATABASE_FILE, "r");
            if (database == NULL) {
                perror("fopen");
                close(client_fd);
                pthread_exit(NULL);
            }
            struct record found_record;
            pthread_mutex_lock(&mutex);
            while (fread(&found_record, sizeof(found_record), 1, database) == 1) {
                if (found_record.id == request.rd.id) {
                    // Send found record as success response
                    struct msg response = {SUCCESS, found_record};
                    if (send(client_fd, &response, sizeof(response), 0) < 0) {
                        perror("send");
                    }
                    pthread_mutex_unlock(&mutex);
                    fclose(database);
                    // Continue the loop to wait for next request
                    break;
                }
            }

            if (feof(database)) {
                printf("Record not found for ID: %u\n", request.rd.id);
                fclose(database);
                pthread_mutex_unlock(&mutex);

                // Send failure response
                struct msg response = {FAIL};
                if (send(client_fd, &response, sizeof(response), 0) < 0) {
                    perror("send");
                }
            }
        } else {
            // Invalid request type
            close(client_fd);
            pthread_exit(NULL);
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "msg.h"

void put_record(int sock_fd);
void get_record(int sock_fd);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *hostname = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    // Open socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    int choice;
    do {
        printf("Enter your choice (1 to put, 2 to get, 0 to quit): ");
        scanf("%d", &choice);
        getchar();  // Consume newline character

        switch (choice) {
            case PUT:
                put_record(sock_fd);
                break;
            case GET:
                get_record(sock_fd);
                break;
            case 0:
                break;
            default:
                printf("Invalid choice\n");
                break;
        }
    } while (choice != 0);

    // Close socket
    close(sock_fd);

    return 0;
}

void put_record(int sock_fd) {
    // Prompt user for record details
    struct record new_record;
    printf("Enter the name: ");
    fgets(new_record.name, MAX_NAME_LENGTH, stdin);
    new_record.name[strcspn(new_record.name, "\n")] = '\0';  // Remove newline character
    printf("Enter the id: ");
    scanf("%u", &new_record.id);
    getchar();  // Consume newline character

    // Send PUT message to server
    struct msg request = {PUT, new_record};
    if (send(sock_fd, &request, sizeof(request), 0) < 0) {
        perror("send");
        return;
    }

    // Receive response from server
    struct msg response;
    if (recv(sock_fd, &response, sizeof(response), 0) < 0) {
        perror("recv");
        return;
    }

    // Print response
    if (response.type == SUCCESS) {
        printf("Put success.\n");
    } else {
        printf("Put failed.\n");
    }
}

void get_record(int sock_fd) {
    // Prompt user for record id
    struct record search_record;
    printf("Enter the id: ");
    scanf("%u", &search_record.id);
    getchar();  // Consume newline character

    // Send GET message to server
    printf("Sending GET request for ID: %u\n", search_record.id);
    struct msg request = {GET, search_record};
    if (send(sock_fd, &request, sizeof(request), 0) < 0) {
        perror("send");
        return;
    }

    // Receive response from server
    struct msg response;
    if (recv(sock_fd, &response, sizeof(response), 0) < 0) {
        perror("recv");
        return;
    }

    // Print response
    if (response.type == SUCCESS) {
        printf("name: %s\n", response.rd.name);
        printf("id: %u\n", response.rd.id);
    } else {
        printf("Get failed.\n");
    }
}

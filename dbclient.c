#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#define BUF 256
#define MAX_NAME_LEN 128

void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);

void put_record(int socket_fd);
void get_record(int socket_fd);

int
main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }

  printf("Ready to communicate with %s:%s\n", argv[1], argv[2]);


  int choice; 
  do{
    printf("Enter your choice (1 to put, 2 to get, 0 to quit): ");
    scanf("%d", &choice);
    getchar(); // consume the newline

    switch(choice){
      case 1:
        put_record(socket_fd);
      break;
      case 2:
        get_record(socket_fd);
      break;
      case 0:
        break;
      default:
        printf("Invalid choice\n");
        break;
    }
  }while(choice != 0);

  close(socket_fd);
  return EXIT_SUCCESS;
}

void
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }


  *ret_fd = socket_fd;
  return 1;
}

void put_record(int socket_fd){
  struct record new_record;
  printf("Enter the name: ");
  fgets(new_record.name, MAX_NAME_LEN, stdin);
  new_record.name[strlen(new_record.name) - 1] = "\0";

  printf("Enter the id: ");
  char input[MAX_NAME_LEN];
  fgets(input, MAX_NAME_LEN, stdin);
  new_record.id = atoi(input);

  struct msg request = {PUT, new_record};
  if(send(socket_fd, &request, sizeof(request), 0) <= 0){
    printf("Error sending request\n");
    return;
  }

  struct msg response; 
  if(recv(socket_fd, &response, sizeof(response), 0) <= 0){
    printf("Error receiving response\n");
    return;
  }
  if(response.type == SUCCESS){
    printf("Put success\n");
  }
  else if (response.type == FAIL){
    printf("put failed\n");
  }
}

void get_record(int socket_fd){
  uint32_t id;
  printf("Enter the id: ");

  char input[MAX_NAME_LEN];
  fgets(input, MAX_NAME_LEN, stdin);
  id = atoi(input);

  struct record search_record;
  search_record.id = id;
  struct msg request = {GET, search_record};
  if(send(socket_fd, &request, sizeof(request), 0) <= 0){
    printf("Error sending request\n");
    return;
  }

  struct msg response; 
  if(recv(socket_fd, &response, sizeof(response), 0) <= 0){
    printf("Error receiving response\n");
    return;
  }

  if(response.type == SUCCESS){
    printf("name: %s\n", search_record.name);
    printf("id: %d\n", search_record.id);
  }
  else if (response.type == FAIL){
    printf("get failed");
  }
} 

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>

int create_server_socket();
int create_client_socket();

void bind_socket(int socket, uint16_t);
void listen_socket(int socket);
void accept_connection(int socket);
int connect(int socket, uint32_t ip, uint16_t port);

void die(const char *message);
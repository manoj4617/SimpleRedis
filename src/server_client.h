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
#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <map>
#include <string>


const size_t k_max_msg = 4096;

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,      // Mark the connection for deletion
};

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

struct Conn {
    int fd = -1;
    uint32_t state = 0;
    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

int create_server_socket();
int create_client_socket();

void bind_socket(int socket, uint16_t);
void listen_socket(int socket);
void accept_connection(int socket);
int connect(int socket, uint32_t ip, uint16_t port);

int32_t send_req(int fd, std::vector<std::string> &cmd);
int32_t read_res(int fd);

void die(const char *message);
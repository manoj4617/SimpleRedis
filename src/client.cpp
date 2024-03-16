#include "server_client.h"
#include "parser.h"

static void die(const char *message){
    int err = errno;
    fprintf(stderr,"[%d] %s\n",err,message);
}

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}
/**
 * Creates a client socket by calling the socket function with AF_INET and SOCK_STREAM parameters.
 *
 * @return the created socket
 *
 * @throws ErrorType if the socket creation fails
 */
int create_client_socket(){

    //Creating a TCP server socket
    int sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock < 0){
        die("socket()");
    }

    return sock;
}

int32_t send_req(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    return write_all(fd, wbuf, 4 + len);
}

int32_t read_res(int fd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}

/**
 * Establishes a connection to a specified IP address and port using the given socket.
 *
 * @param socket the socket to use for the connection
 * @param ip the IP address to connect to
 * @param port the port number to connect to
 *
 * @return void
 *
 * @throws ErrorType if the connection attempt fails
 */
int connect(int socket, uint32_t ip, uint16_t port){
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);
    if(connect(socket, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        die("connect()");
    }

}

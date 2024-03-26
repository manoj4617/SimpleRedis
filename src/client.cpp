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

int32_t send_req(int fd, std::vector<std::string> &cmd) {
    uint32_t len = 4;

    for(const std::string &s : cmd){
        len += 4 + s.size();
    }
    if(len > k_max_msg){
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);

    size_t cur = 8;
    for(const std::string &s : cmd){
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur+4], s.data(), s.size());
        cur += 4 + s.size();
    }

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

    uint32_t rescode = 0;
    if(len < 4){
        msg("bad response");
        return -1;
    }

    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);

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

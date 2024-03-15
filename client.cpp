#include "server_client.h"

static void die(const char *message){
    int err = errno;
    fprintf(stderr,"[%d] %s\n",err,message);
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

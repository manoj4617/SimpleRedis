#include "server_client.h"

static void die(const char *message){
    int err = errno;
    fprintf(stderr,"[%d] %s\n",err,message);
}

int create_client_socket(){

    //Creating a TCP server socket
    int sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock < 0){
        die("socket()");
    }

    return sock;
}

int connect(int socket, uint32_t ip, uint16_t port){
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);
    if(connect(socket, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        die("connect()");
    }

}

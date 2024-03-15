#include "server_client.h"

static void die(const char *message){
    int err = errno;
    fprintf(stderr,"[%d] %s\n",err,message);
}

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void do_something(int client_sock){

    char buffer[256];
    size_t bytes_read = read(client_sock,buffer, sizeof(buffer));
    if(bytes_read < 0){
        msg("read() error");
        return;
    }

    printf("Client says: %s\n", buffer);
    write(client_sock, buffer, sizeof(buffer));
}

int create_server_socket(){

    //Creating a TCP server socket
    int sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock < 0){
        die("socket()");
    }

    
    //This enables the SO_REUSEADDR option on the server socket so that when the program ends, 
    //the port can be reused quickly instead of waiting for the OS to timeout the socket. 
    //This allows restarting the server program on the same port without waiting.
    int val = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0){
        die("setsockopt()");
    }

    return sock;
}

void bind_socket(int server_sock, uint16_t port){

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        die("bind()");
    }
}

void listen_socket(int server_sock){
    if(listen(server_sock, SOMAXCONN) < 0){
        die("listen()");
    }
}

void accept_connection(int server_sock){

    while(true){

        struct sockaddr_in client_addr;
        socklen_t sock_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &sock_len);
        if(client_sock < 0){
            continue;
        }

        do_something(client_sock);
        close(client_sock);
    }
}
#include "server_client.h"

int main(){
    int server_fd = create_server_socket();
    bind_socket(server_fd, 8080);
    listen_socket(server_fd);
    accept_connection(server_fd);
    return 0;

}
#include "server_client.h"

int main(){
    int client_fd = create_client_socket();

    int rv = connect(client_fd, INADDR_LOOPBACK, 8080);

    char msg[] = "Hello";
    write(client_fd, msg, sizeof(msg));

    char buffer[256];
    size_t bytes_read = read(client_fd, buffer, sizeof(buffer));
    if(bytes_read < 0){
        die("read() error");
    }

    printf("Server says: %s\n", buffer);
    close(client_fd);

    return 0;
}
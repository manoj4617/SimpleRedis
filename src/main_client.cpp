#include "server_client.h"
#include "parser.h"
int main(int argc, char **argv){
    int client_fd = create_client_socket();

    int rv = connect(client_fd, INADDR_LOOPBACK, 8080);

    std::vector<std::string> cmd;

    for(int i = 1; i < argc; ++i){
        cmd.push_back(argv[i]);
    }

    int32_t err =  send_req(client_fd, cmd);
    if(err)
        goto L_DONE;
    err = read_res(client_fd);
    if(err)
        goto L_DONE;

L_DONE:    
    close(client_fd);
    return 0;
}
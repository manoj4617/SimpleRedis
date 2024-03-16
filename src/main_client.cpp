#include "server_client.h"
#include "parser.h"
int main(){
    int client_fd = create_client_socket();

    int rv = connect(client_fd, INADDR_LOOPBACK, 8080);

    const char *query_list[4] = {   "this is client",
                                    "sending a request",
                                    "please accept the request",
                                    "good bye"
                                };

    for(size_t i=0;i<4;++i){
        int32_t err =  send_req(client_fd, query_list[i]);
        if(err)
            goto L_DONE;
    }

    for(size_t i = 0;i<4;++i){
        int32_t err = read_res(client_fd);
        if(err)
            goto L_DONE;
    }

L_DONE:    
    close(client_fd);
    return 0;
}
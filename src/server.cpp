#include "server_client.h"
#include "parser.h"

static void state_req(Conn *conn);
static void state_res(Conn *conn);

/**
 * A function that prints an error message along with the corresponding error number to the standard error stream.
 *
 * @param message a pointer to a constant character string containing the error message to be printed
 *
 * @return void
 *
 * @throws N/A
 */
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

/**
 * Generate a server socket for TCP connections.
 *
 * @return the created server socket
 *
 * @throws Error if the socket creation or setting socket options fail
 */
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

/**
 * Binds a socket to a specified port.
 *
 * @param server_sock the server socket to bind
 * @param port the port to bind the socket to
 *
 * @throws ErrorType if the binding fails
 */
void bind_socket(int server_sock, uint16_t port){

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("bind");
        die("bind()");
    }
}

/**
 * Function to listen on a socket.
 *
 * @param server_sock the server socket to listen on
 *
 * @return void
 *
 * @throws Error if listen() fails
 */
void listen_socket(int server_sock){
    if(listen(server_sock, SOMAXCONN) < 0){
        die("listen()");
    }
}

/*
 * Function: parse_req
 * 
 * This function takes a pointer to a byte array, the length of the array, and a reference to a vector of strings.
 * 
 * The purpose of this function is to parse a request that follows a specific format. The format is:
 * 
 * | 4-byte little-endian integer representing the number of strings | sequence of strings 
 * (each preceded by a 4-byte integer representing the length of the string)
 * 
 * The function will return 0 if the parsing is successful, and -1 if there is an error.
 * 
 * The function will also populate the vector of strings with the parsed strings.
 */

static int32_t parse_req(const uint8_t *data, 
                            size_t len,
                            std::vector<std::string> &out){

    // Check if the length of the data is at least 4 bytes. If not, return -1.
    if(len < 4){
        return -1;
    }

    // Get the number of strings from the first 4 bytes of the data
    uint32_t n = 0;
    memcpy(&n, &data[0], 4);

    // Check if the number of strings is less than or equal to k_max_msg. If not, return -1.
    if(n > k_max_msg){
        return -1;
    }

    // Initialize the position to 4, since the first 4 bytes were used to get the number of strings
    size_t pos = 4;

    // Loop n times, parsing each string
    while(n--){
        // Check if there is enough data to read the length of the string
        if(pos + 4 > len){
            return -1;
        }

        // Get the length of the string
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);

        // Check if there is enough data to read the string
        if(pos + 4 + sz > len){
            return -1;
        }

        // Copy the string into a new std::string object and add it to the vector of strings
        out.push_back(std::string((char*)&data[pos+4], sz));

        // Update the position to the start of the next string
        pos += 4 + sz;
    }

    // Check if the parsing was successful and all data was used. If not, return -1.
    if(pos != len){
        return -1;
    }

    // Return 0 to indicate success
    return 0;
}
static std::map<std::string, std::string> g_map;

/*
 * Function: do_get
 * 
 * This function is called when the server receives a "GET" command.
 * It retrieves the value associated with the given key from a global map.
 * If the key is not found in the map, it returns RES_NX, indicating that
 * the key does not exist. Otherwise, it copies the value into the provided
 * buffer and returns RES_OK.
 * 
 * Parameters:
 * - `cmd`: a vector of strings, where the first element is the command ("GET")
 *          and the second element is the key to retrieve.
 * - `res`: a pointer to a buffer where the value will be copied.
 * - `reslen`: a pointer to the length of the buffer. This function will update
 *            this value with the length of the value.
 * 
 * Returns:
 * - `RES_NX` if the key is not found in the map.
 * - `RES_OK` if the value is found and copied into the buffer.
 */
static uint32_t do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen){
        // Check if the key exists in the map
        if(!g_map.count(cmd[1])){
            // If the key is not found, return RES_NX
            return RES_NX;
        }

        // Retrieve the value associated with the key
        std::string &val = g_map[cmd[1]];

        // Assert that the value is not longer than the maximum allowed message size
        assert(val.size() <= k_max_msg);

        // Copy the value into the provided buffer
        memcpy(res, val.data(), val.size());

        // Update the length of the buffer with the length of the value
        *reslen = (uint32_t)val.size();

        // Return RES_OK to indicate success
        return RES_OK;
}

/*
 * This function is called when the server receives a "SET" command.
 * It sets the value associated with the given key in a global map.
 * The parameters are as follows:
 * - `cmd`: a vector of strings, where the first element is the command ("SET")
 *          and the second element is the key, and the third element is the value.
 * - `res`: a pointer to a buffer, which is not used in this function.
 * - `reslen`: a pointer to an integer, which is also not used in this function.
 * 
 * This function does not return anything, but instead updates the global map.
 *
 * The function updates the global map by associating the value of `cmd[2]` 
 * (the third element of `cmd`) with the key `cmd[1]` (the second element of `cmd`).
 *
 * The function does not perform any error checking, so it is assumed that the
 * caller has properly formatted the `cmd` parameter.
 */
static uint32_t do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen){
        (void)res; // We don't use `res`, so we cast it to void to indicate this.
        (void)reslen; // We also don't use `reslen`, so we cast it to void to indicate this.
        
        // Associate the value of `cmd[2]` with the key `cmd[1]` in the global map.
        g_map[cmd[1]] = cmd[2];
        
        // Return RES_OK to indicate success.
        return RES_OK;
}


/*
 * This function is called when the server receives a "DEL" command.
 * It deletes the key-value pair associated with the given key from the global map.
 * The parameters are as follows:
 * - `cmd`: a vector of strings, where the first element is the command ("DEL")
 *          and the second element is the key.
 * - `res`: a pointer to a buffer, which is not used in this function.
 * - `reslen`: a pointer to an integer, which is also not used in this function.
 * 
 * This function does not return anything, but instead updates the global map.
 *
 * The function updates the global map by erasing the key-value pair from the map,
 * where the key is the second element of the `cmd` vector.
 *
 * The function does not perform any error checking, so it is assumed that the
 * caller has properly formatted the `cmd` parameter.
 */
static uint32_t do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen){
        // We don't use `res` or `reslen`, so we cast them to void to indicate this.
        (void)res;
        (void)reslen;
        
        // Erase the key-value pair from the global map, where the key is the second element of `cmd`.
        g_map.erase(cmd[1]);
        
        // Return RES_OK to indicate success.
        return RES_OK;
}

static int32_t cmd_is(const std::string &word, const char *cmd){
    return 0 == strcasecmp(word.c_str(), cmd);
}


/**
 * This function processes a single request received from a client.
 *
 * @param req The buffer containing the received request.
 * @param reqlen The length of the request buffer.
 * @param rescode Pointer to store the result code of the request.
 * @param res Pointer to store the response buffer.
 * @param reslen Pointer to store the length of the response buffer.
 *
 * @return Returns 0 on success, -1 on error.
 *
 * Steps involved in processing a request:
 * 1. Parse the request buffer into a vector of strings, where each string
 *    represents a command or a parameter.
 * 2. Check if the parsed request has a valid format based on the number
 *    of elements in the vector and the commands.
 * 3. If the request is valid, dispatch the request to the appropriate
 *    handler function (do_get, do_set, do_del) based on the command.
 * 4. The appropriate handler function performs the operation and stores the
 *    result in the response buffer.
 * 5. If the request is not valid, set the result code to RES_ERR and
 *    store the error message in the response buffer.
 */
static int32_t do_request(const uint8_t *req, uint32_t reqlen,
                            uint32_t *rescode, uint8_t *res,
                            uint32_t *reslen){
    
    // Parse the request buffer into a vector of strings
    std::vector<std::string> cmd;
    if(0 != parse_req(req, reqlen, cmd)){
        // If parsing fails, log a message and return an error
        msg("bad req");
        return -1;
    }

    // Check if the parsed request has a valid format
    if(cmd.size() == 2 && cmd_is(cmd[0], "get")){
        // Dispatch the request to the appropriate handler function
        *rescode = do_get(cmd, res, reslen);
    } else if(cmd.size() == 3 && cmd_is(cmd[0], "set")){
        *rescode = do_set(cmd, res, reslen);
    } else if(cmd.size() == 2 && cmd_is(cmd[0], "del")){
        *rescode = do_del(cmd, res, reslen);
    } else {
        // If the request format is invalid, set the result code to RES_ERR
        // and store an error message in the response buffer
        *rescode = RES_ERR;
        const char *msg = "Unknown command";
        strcpy((char*)res, msg);
        *reslen = strlen(msg);
        return 0;
    }

    // Return 0 to indicate success
    return 0;
}

/**
 * Tries to parse and process a single request from the connection buffer.
 *
 * Checks if there is enough data in the read buffer to contain a full request.
 * A request starts with a 4 byte length header indicating the total message size.
 *
 * If there is not enough data yet, returns false to indicate the outer loop
 * should retry on the next iteration after more data is read.
 *
 * If there is enough data, parses the length and verifies it is within the
 * allowed maximum.
 *
 * Prints the request payload and generates an echo response by copying the
 * request to the write buffer.
 *
 * Removes the processed request data from the read buffer and updates the buffer
 * size.
 *
 * Changes the connection state to response and calls the response handler.
 *
 * Returns false if the request was fully handled, which continues the outer loop.
 * Returns true if the connection state changed back to requesting more data,
 * which breaks the outer loop.
 *
 * @param conn The connection containing the read/write buffers.
 * @return bool Whether the outer loop should continue or break.
 */
static bool try_one_request(Conn *conn){
    // try to parse a request from the buffer
    if(conn->rbuf_size < 4){
        // not enough data in the buffer wil retry in the next iteration
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    
    if(len > k_max_msg){
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    if(len + 4 > conn->rbuf_size){
        return false;
    }

    // got one request
    // printf("Client says: %.*s\n", len, &conn->rbuf[4]);

    uint32_t rescode = 0;
    uint32_t wlen = 0;
    uint32_t err = do_request(&conn->rbuf[4], len, &rescode, &conn->wbuf[4 + 4], &wlen);

    if(err){
        conn->state = STATE_END;
        return false;
    }

    wlen += 4;
    // generate echoing response
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    conn->wbuf_size = 4 + wlen;

    // remove the request from the buffer
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain){
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    // change the state
    conn->state = STATE_RES;
    state_res(conn);

    // continue the outer loop if the request was fully processed
    return (conn->state == STATE_REQ);
}

/**
 * Tries to fill the receive buffer for the given connection by reading
 * from the socket.
 *
 * It will read as much data as possible without blocking into the
 * receive buffer.
 *
 * The receive buffer has a fixed capacity, so it will return false
 * if the buffer is already full.
 *
 * It also watches out for EOF and errors on the socket. If EOF is
 * received, it will transition the connection state to CLOSED. For
 * errors, it will transition to ERROR state.
 *
 * @param conn The connection object to fill the receive buffer for
 * @return True if the buffer has data ready for processing, false if
 *         buffer is empty or error occurred.
 */
static bool try_fill_buffer(Conn *conn){
    // try to fill the buffer
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;

    do{
       size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
       rv = read(conn->fd,&conn->rbuf[conn->rbuf_size], cap); 
    }while(rv < 0 && errno == EINTR);

    if(rv < 0  && errno == EAGAIN){
        return false;
    }

    if(rv < 0){
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }

    if(rv == 0){
        if(conn->rbuf_size > 0){
            msg("unexpected EOf");
        }
        else{
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // try to process requests one by one
    while(try_one_request(conn)){}
    return (conn->state == STATE_REQ);
}

/**
 * Tries to flush the write buffer for the given connection.
 *
 * This will attempt to write any remaining unsent data in the write buffer
 * to the socket. It will repeatedly call write() in a loop until either:
 *
 * - All data is successfully written (wbuf_sent == wbuf_size)
 * - An error occurs other than EAGAIN (e.g. broken pipe)
 * - write() returns EAGAIN, indicating the socket is not ready for more writes.
 *
 * On success (all data flushed), it will reset the write buffer and
 * transition the connection state back to STATE_REQ.
 *
 * On a non-EAGAIN error, the connection state is set to STATE_END.
 *
 * Returns:
 *  - true if there is still unflushed data after attempting to write.
 *  - false if the buffer has been completely flushed.
 */
static bool try_flush_buffer(Conn *conn){
    ssize_t rv = 0;
    do{
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);

    }while(rv < 0 && errno == EAGAIN);

    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }

    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);

    if(conn->wbuf_sent == conn->wbuf_size){
        // response was full sent
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false; 
    }

    //still got some data in wbuf
    return true;
}

static void state_req(Conn *conn){
    while(try_fill_buffer(conn)){}
}

static void state_res(Conn *conn){
    while (try_flush_buffer(conn)) {}
}

static void fd_set_nb(int fd){
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(errno){
        die("fcnlt error");
        return;
    }
    flags |= O_NONBLOCK;
    errno = 0;
    
    (void)fcntl(fd, F_SETFL,flags);
    if(errno){
        die("fcnlt error");
        return;
    }
}

static void connection_io(Conn *conn){
    if(conn->state == STATE_REQ){
        state_req(conn);
    }
    else if(conn->state == STATE_RES){
        state_res(conn);
    }
    else {
        assert(0);
    }

}

static void conn_put(std::vector<Conn*> &fd2conn, Conn *conn){
    if(fd2conn.size() <= (size_t)conn->fd){
        fd2conn.resize(conn->fd+1);
    }
    fd2conn[conn->fd] = conn;
}

static int32_t accept_new_connection(std::vector<Conn *> &fd2conn, int fd){
    // accept 
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if(connfd < 0){
        die("accept()");
        return -1;
    }

    // set the new connection tfd to nonblocking mode
    fd_set_nb(connfd);

    // Create a connection struct
    Conn *conn = (Conn*) malloc(sizeof(Conn));
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);

    return 0;
}

/**
 * Accepts new connections on the given server socket and handles IO with
 * existing connections.
 *
 * This uses poll() to monitor sockets for events and non-blocking IO.
 *
 * Parameters:
 * - server_sock: The listening server socket to accept connections on.
 *
 * It maintains a map (fd2conn) from socket FDs to Conn objects representing
 * each connection.
 *
 * The main loop polls for events and calls helper functions to handle:
 * - Accepting new connections
 * - Reading requests from connections in STATE_REQ
 * - Writing responses to connections in STATE_RES
 *
 * If a socket has an error or closes, its Conn is freed.
 */
void accept_connection(int server_sock){

    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn;

    // set the listen fd to non-blocking
    fd_set_nb(server_sock);

    // event loop
    std::vector<pollfd> poll_args;
    while(true){

        // prepare arguments for poll
        poll_args.clear();

        // the listening fd is put in the first position
        struct pollfd pfd = {server_sock, POLLIN, 0};

        poll_args.push_back(pfd);

        // connection fds
        for(Conn* con : fd2conn){
            if(!con)
                continue;
            struct pollfd pfd = {};
            pfd.fd = con->fd;
            pfd.events = (con->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // poll for activ fds
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if(rv < 0){
            die("poll");
        }

        // process actiev connections
        for(size_t i = 1; i<poll_args.size();++i){
            if(poll_args[i].revents){
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if(conn->state == STATE_END){
                    // client closed normally or something bad happened
                    // destroy this connection
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        if(poll_args[0].revents){
            (void)accept_new_connection(fd2conn,server_sock);
        }
    }
}
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
    printf("Client says: %.*s\n", len, &conn->rbuf[4]);

    // generate echoing response
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

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
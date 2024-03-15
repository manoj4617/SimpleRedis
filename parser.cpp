#include "parser.h"
#include "server_client.h"

/*
Our server will be able to process multiple requests from a client, 
so we need to implement some sort of “protocol” to at least split 
requests apart from the TCP byte stream. 
The easiest way to split requests apart is to declare how long the 
request is at the beginning of the request.

4-byte little-endian int for length of the request and variable length request 
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------

*/
static void msg(char *msg){
    fprintf(stderr,msg);
}

static int32_t read_full(int fd, char *buf, size_t n){
    while(n > 0){
        ssize_t rv = read(fd, buf, n);
        if(rv <= 0){
            return -1;
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n){
    while(n > 0){
        ssize_t rv = write(fd, buf, n);
        if(rv <= 0){
            return -1;
        }

        assert((size_t)rv <= n);
        n -=(size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd){
    // 4 bytes header
    char buf[4 + k_max_msg + 1];
    errno = 0;

    int32_t err = read_full(connfd, buf, 4);

    if(err){
        if(errno == 0){
            msg("EOF");
        }
        else{
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, buf,4);
    if(len > k_max_msg){
        msg("too long");
        return -1;
    }

    //request body
    err = read_full(connfd,&buf[4],len);
    if(err){
        msg("read() error");
        return err;
    }

    //do something
    buf[4 + len] = '\0';
    printf("Client says: %s\n", &buf[4]);

    //reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);

    return write_all(connfd, wbuf, 4 + len);
}

static int32_t query(int fd, const char *text){
    uint32_t len = (uint32_t)strlen(text);
    
    if(len > k_max_msg){
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, 4);

    if(int32_t err = write_all(fd, wbuf, 4 + len)){
        return err;
    }

    // 4 byte header
    char rbuf[4 + k_max_msg];
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

    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}
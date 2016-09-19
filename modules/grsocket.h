#ifndef GR_SOCKET
#define GR_SOCKET
#include "../types/object.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

class Socket: public Object {
    public:
    Socket();
    int listen_fd;
};

class SocketConnection: public Object {
    public:
    int comm_fd;
    SocketConnection(int comm_fd);
};

void init_grsocket();

#endif

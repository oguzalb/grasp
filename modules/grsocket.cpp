#include "grmysql.h"
#include "../vm.h"

extern Class *str_type;
extern Object *none;
extern Class *class_type;
extern Class *int_type;
Class *socket_connection_type;
Class *socket_type;
extern Class *exception_type;
extern std::unordered_map<string, Object*> *builtins;
extern std::vector<Object*> gstack;
extern Object *none;

SocketConnection::SocketConnection(int comm_fd) {
    this->type = socket_connection_type;
    this->comm_fd = comm_fd;
}

Socket::Socket() {
    this->type = socket_type;
}

void grsocket_new() {
    if (POP_TYPE(class_type) == NULL)
        return;
    PUSH(new Socket());
}

void grsocket_init() {
    Socket *sock = (Socket *)POP_TYPE(socket_type);
    if (sock == NULL)
        return;
    sock->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    PUSH(none);
}

void grsocket_bind() {
    Int *port = (Int *)POP_TYPE(int_type);
    if (port == NULL)
        {POP();return;}
    Socket *sock = (Socket *)POP_TYPE(socket_type);
    if (sock == NULL)
        {return;}
    struct sockaddr_in servaddr;
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port->ival);
    int bind_result = bind(sock->listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bind_result < 0) {
        newerror_internal("ERROR on bind", exception_type);
        exit(1);
    }
    listen(sock->listen_fd, 10);
    if (sock->listen_fd < 0) {
        newerror_internal("ERROR on listen", exception_type);
        exit(1);
    }
 
    PUSH(none);
}

void grsocket_accept() {
    Socket *sock = (Socket *)POP_TYPE(socket_type);
    if (sock == NULL)
        return;
    int comm_fd = accept(sock->listen_fd, (struct sockaddr*) NULL, NULL);
    if (comm_fd < 0) {
        newerror_internal("ERROR on accept", exception_type);
        exit(1);
    }
    PUSH(new SocketConnection(comm_fd));
}

void grsocket_connect() {
    Int *port = (Int *)POP_TYPE(int_type);
    if (port == NULL)
        {POP();POP();return;}
    String *address = (String *)POP_TYPE(str_type);
    if (address == NULL)
        {POP();return;}
    Socket *sock = (Socket *)POP_TYPE(socket_type);
    if (sock == NULL)
        return;

    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof servaddr);
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port->ival);
    inet_pton(AF_INET, address->sval.c_str(), &(servaddr.sin_addr));
    connect(sock->listen_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    PUSH(none);
}

void grsocket_sendall() {
    String *data = (String *)POP_TYPE(str_type);
    if (data == NULL)
        {POP();return;}
    Socket *sock = (Socket *)POP_TYPE(socket_type);
    if (sock == NULL)
        return;
    write(sock->listen_fd, data->sval.c_str(), data->sval.size());
    PUSH(none);
}

void grsocket_connection_recv() {
    SocketConnection *con = (SocketConnection *)POP_TYPE(socket_connection_type);
    if (con == NULL)
        return;
    char str[256];
    int count = 0;
    string data = "";
    while((count = read(con->comm_fd, str, sizeof(str)-1)) > 0)
    {
        str[count] = '\0';
        data += str;
    }
    PUSH(new String(data));
}

void init_grsocket() {
    Module *grsocket = new Module(NULL);
    socket_type = new Class("socket", grsocket_new, 1);
    socket_type->setmethod("__init__", grsocket_init, 1);
    socket_type->setmethod("bind", grsocket_bind, 2);
    socket_type->setmethod("accept", grsocket_accept, 1);
    socket_type->setmethod("connect", grsocket_connect, 3);
    socket_type->setmethod("sendall", grsocket_sendall, 2);
    grsocket->setfield("socket", socket_type);
    
    socket_connection_type = new Class("SocketConnection", NULL, 1);
    socket_connection_type->setmethod("recv", grsocket_connection_recv, 1);
    grsocket->setfield("SocketConnection", socket_connection_type);
    (*builtins)["socket"] = grsocket;
}

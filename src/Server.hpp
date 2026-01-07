#pragma once
#include <vector>
#include <poll.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include "Connection.hpp"
#include "KeyValueStore.hpp"

// Typedefs
typedef struct sockaddr_in SocketAddressIPV4;
typedef struct sockaddr SocketAddress;
typedef socklen_t SocketAddressSize;
typedef struct pollfd PollFD;


class Server {
public:
    Server(int port);
    void run(); // Starts the infinite loop

private:
    int server_fd;
    int port;
    KeyValueStore kv_store;
    std::vector<Connection*> fd_to_connection;

    void accept_new_connection();
};
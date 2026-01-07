#pragma once
#include <vector>
#include <poll.h>
#include "Connection.hpp"

// Typedefs
typedef struct sockaddr_in SocketAddressIPV4;
typedef struct sockaddr SocketAddress;
typedef socklen_t SocketAddressSize;
typedef struct pollfd PollFD;

//======================   Constants start  ======================   
const size_t NOPOSITION = std::string::npos;
//======================   Constants end    ======================   

class Server {
public:
    Server(int port);
    void run(); // Starts the infinite loop

private:
    int server_fd;
    int port;
    std::vector<Connection*> fd_to_connection;

    void accept_new_connection();
};
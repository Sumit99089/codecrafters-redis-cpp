#include<iostream>
#include <sys/socket.h> // for socket(), setsockopt(), listen(), bind(), read(), accept(), write(), close() and all the defined variables like AF_INET
#include <netinet/in.h> // For sockaddr_in, AF_INET, and INADDR_ANY
#include<unistd.h>
#include<string>
#include<cstring>
#include<fcntl.h>
#include<poll.h>
#include<vector>


//Constants
#define PORT 6379
#define NOPOSITION std::string::npos
//TYpedef
typedef struct sockaddr_in SocketAddressIPV4;
typedef struct sockaddr SocketAddress;
typedef socklen_t SocketAddressSize; // socklen_t is not a struct in itself. its also a typeof a struct. just like we cant write typedef stuct SocketAddress abc
typedef struct pollfd PollFD;


int set_fd_nonblocking(int fd){ 
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1){
        return -1;
    }
    flags = flags | O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}


int main(int argc, char **argv) {

    std::cout<< std::unitbuf;
    std::cerr<< std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd<0){
        std::cerr<<"Failed to create socket server\n";
        return 1;  //returning 1 from main is failure, 0 is for success
    }

    int reuse = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        std::cerr<<"Set socket option to Reuse address failed\n";
        return 1;
    }

    if(set_fd_nonblocking(server_fd) == -1){
        std::cerr<<"Failed to set server socket Non-Blocking\n";
        return 1;
    }

    SocketAddressIPV4 server_address;
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_fd, (SocketAddress *) &server_address, sizeof(server_address))!=0){
        std::cerr<<"Socket binding failed\n";
        return 1;
    }

    int request_queue_size = 5;
    if(listen(server_fd, request_queue_size)!=0){
        std::cerr<<"Listen failed\n";
        return 1;
    }




    SocketAddress client_address;
    int client_address_length = sizeof(client_address);
    
    int client_fd = accept(server_fd,(SocketAddress *) &client_address ,(SocketAddressSize *) client_address_length); //blocking call
    std::cout<<"Client connected\n";

    char buffer[1024] = {0};

    //Handle Client
    while(true){
        int bytes_read  = read(client_fd, buffer, sizeof(buffer)); //blocking call

        if(bytes_read < 0){
            std::cerr<<"Error reading client message\n";
            break;
        }
        if(bytes_read == 0){
            std::cout<<"Client has disconnected\n";
            break;
        }

        std:: string client_request(buffer, bytes_read);

        if(client_request.find("PING") != NOPOSITION){
            const char *response = "+PONG\r\n";
            int result = send(client_fd, response, strlen(response), 0); //blocking call
            std::cout<<strlen(response)<<" bytes sent to client\n";

            if(result == -1){
                std::cerr<<"Error writing to client\n";
            }
        }
    }
    close(client_fd);
    //Handle Client
    return 0;
}


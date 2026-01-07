#include "Server.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Constructor
Server::Server(int port)
{
    this->port = port;
    // Create the server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        std::cerr << "Failed to create socket server\n";
        exit(1);
    }

    this->server_fd = server_fd;

    int reuse = 1;
    // Set socket option to reuse address to avoid 'Address already in use' errors
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        std::cerr << "Set socket option to Reuse address failed\n";
        exit(1);
    }

    // Set the server socket to non-blocking mode
    if (set_fd_nonblocking(server_fd) == -1)
    {
        std::cerr << "Failed to set server socket Non-Blocking\n";
        exit(1);
    }

    SocketAddressIPV4 server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the IP and Port
    if (bind(server_fd, (SocketAddress *)&server_address, sizeof(server_address)) != 0)
    {
        std::cerr << "Socket binding failed\n";
        exit(1);
    }

    int request_queue_size = SOMAXCONN;
    // Start listening for incoming connections
    if (listen(server_fd, request_queue_size) != 0)
    {
        std::cerr << "Listen failed\n";
        exit(1);
    }
}

void Server::run()
{
    std::vector<PollFD> poll_arguments;

    // Event Loop
    while (true)
    {
        poll_arguments.clear();

        PollFD server_poll_fd;
        server_poll_fd.events = POLLIN;
        server_poll_fd.fd = server_fd;
        server_poll_fd.revents = 0;

        // Add server socket to poll arguments
        poll_arguments.push_back(server_poll_fd);

        for (Connection *connection : fd_to_connection)
        {
            if (connection == NULL)
            {
                continue;
            }

            PollFD client_poll_fd;
            client_poll_fd.fd = connection->fd;
            client_poll_fd.events = POLLERR; // Monitor errors by default
            client_poll_fd.revents = 0;

            if (connection->want_read)
            {
                client_poll_fd.events = client_poll_fd.events | POLLIN;
            }
            if (connection->want_write)
            {
                client_poll_fd.events = client_poll_fd.events | POLLOUT;
            }
            // Add client socket to poll arguments
            poll_arguments.push_back(client_poll_fd);
        }

        // Wait for events on any of the sockets
        int return_value = poll(poll_arguments.data(), (nfds_t)poll_arguments.size(), -1);
        if (return_value < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                std::cerr << "Poll failed\n";
                exit(1);
            }
        }

        // Check if there is a new connection request on the server socket
        if (poll_arguments[0].revents & POLLIN)
        {
            accept_new_connection();
        }

        // Iterate through client sockets to handle events
        for (size_t i = 1; i < poll_arguments.size(); ++i)
        {

            uint32_t ready = poll_arguments[i].revents;
            if (ready == 0)
            {
                continue;
            }

            int fd = poll_arguments[i].fd;
            Connection *connection = fd_to_connection[fd];

            if (!connection)
                continue;

            // Handle read event
            if ((ready & POLLIN) > 0)
            {
                connection->handle_read();
            }

            // Handle write event
            if (!connection->want_close && (ready & POLLOUT) > 0)
            {
                connection->handle_write();
            }

            // Handle errors or close request
            if ((ready & POLLERR) || connection->want_close)
            {
                // Clear the connection from the map and free memory
                fd_to_connection[connection->fd] = NULL;
                delete connection;
            }
        }
    }
}

void Server::accept_new_connection()
{
    SocketAddressIPV4 client_address;
    SocketAddressSize client_address_length = sizeof(client_address);

    // Accept the new incoming connection
    int client_fd = accept(server_fd, (SocketAddress *)&client_address, &client_address_length);

    if (client_fd < 0)
    {
        std::cerr<<"Failed to accept the client\n";
        return;
    }

    char *ip_string = inet_ntoa(client_address.sin_addr);
    uint16_t port = ntohs(client_address.sin_port);

    std::cout << "Client connected\n";
    std::cout << "IP:" << ip_string << "\n";
    std::cout << "PORT:" << port << "\n";

    // Set the new client socket to non-blocking mode
    set_fd_nonblocking(client_fd);

    Connection *connection = new Connection(client_fd);

    if (connection)
    {
        if (fd_to_connection.size() <= (size_t)connection->fd)
        {
            fd_to_connection.resize(connection->fd + 1);
        }
        // Map the file descriptor to the connection object
        fd_to_connection[connection->fd] = connection;
    }
}
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <arpa/inet.h>
#include <algorithm>
#include <cerrno>

// Constants
#define PORT 6379
#define NOPOSITION std::string::npos

// Typedefs
typedef struct sockaddr_in SocketAddressIPV4;
typedef struct sockaddr SocketAddress;
typedef socklen_t SocketAddressSize;
typedef struct pollfd PollFD;
typedef struct Connection Connection;

struct Connection
{
  int fd = -1;

  bool want_read = false;
  bool want_write = false;
  bool want_close = false;

  std::vector<unsigned char> incoming_message;
  std::vector<unsigned char> outgoing_message;
};

// ---------------------------------------------------------
// FORWARD DECLARATIONS 
// ---------------------------------------------------------
static void handle_read(Connection *connection);
static void handle_write(Connection *connection);
static bool try_one_request(Connection *connection);
static int set_fd_nonblocking(int fd);
static Connection *handle_client_accept(int server_fd);

// ---------------------------------------------------------
// IMPLEMENTATION
// ---------------------------------------------------------

static void handle_read(Connection *connection)
{
  unsigned char buffer[1024 * 64] = {0};
  int bytes_read = read(connection->fd, buffer, sizeof(buffer));

  if (bytes_read < 0)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      return;
    }
    else
    {
      connection->want_close = true;
      return;
    }
  }

  if (bytes_read == 0)
  {
    if (connection->incoming_message.size() == 0)
    {
      std::cout << "Client Closed\n";
    }
    else
    {
      std::cout << "Unexpected EOF\n";
    }
    connection->want_close = true;
    return;
  }

  connection->incoming_message.insert(
      connection->incoming_message.end(),
      buffer,
      buffer + bytes_read);

  while (try_one_request(connection) == true)
  {
  }

  if (connection->outgoing_message.size() > 0)
  {
    connection->want_read = false;
    connection->want_write = true;
    return handle_write(connection); // Calls the declared function
  }
}

static void handle_write(Connection *connection)
{
  int result = send(connection->fd, connection->outgoing_message.data(), connection->outgoing_message.size(), 0);

  if (result < 0)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      return;
    }
    else
    {
      connection->want_close = true;
      return;
    }
  }

  // FIX: Only erase the bytes that were actually sent (result), not the whole buffer.
  // If 'send' couldn't send everything at once, we must keep the rest for later.
  connection->outgoing_message.erase(
      connection->outgoing_message.begin(),
      connection->outgoing_message.begin() + result);

  if (connection->outgoing_message.size() == 0)
  {
    connection->want_read = true;
    connection->want_write = false;
  }

  return;
}

static bool try_one_request(Connection *connection)
{
  const char target[] = "PING";

  auto iterator = std::search(
      connection->incoming_message.begin(),
      connection->incoming_message.end(),
      target,
      target + 4);

  if (iterator == connection->incoming_message.end())
  {
    return false;
  }

  const char *response = "+PONG\r\n";

  connection->outgoing_message.insert(
      connection->outgoing_message.end(),
      response,
      response + strlen(response));

  connection->incoming_message.erase(
      connection->incoming_message.begin(),
      iterator + 4 // You had this correct: erase only the processed PING
  );

  return true;
}

static Connection *handle_client_accept(int server_fd)
{
  SocketAddressIPV4 client_address;

  // FIX: Use SocketAddressSize (socklen_t) instead of int for accept()
  SocketAddressSize client_address_length = sizeof(client_address);

  // FIX: No need to cast &client_address_length now that types match
  int client_fd = accept(server_fd, (SocketAddress *)&client_address, &client_address_length);

  if (client_fd < 0)
  {
    return nullptr; // Safety check
  }

  char *ip_string = inet_ntoa(client_address.sin_addr);
  uint16_t port = ntohs(client_address.sin_port);

  std::cout << "Client connected\n";
  std::cout << "IP:" << ip_string << "\n";
  std::cout << "PORT:" << port << "\n";

  set_fd_nonblocking(client_fd);

  Connection *connection = new Connection();
  connection->fd = client_fd;
  connection->want_read = true;

  return connection;
}

static int set_fd_nonblocking(int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
  {
    return -1;
  }
  flags = flags | O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int main(int argc, char **argv)
{

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0)
  {
    std::cerr << "Failed to create socket server\n";
    return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "Set socket option to Reuse address failed\n";
    return 1;
  }

  if (set_fd_nonblocking(server_fd) == -1)
  {
    std::cerr << "Failed to set server socket Non-Blocking\n";
    return 1;
  }

  SocketAddressIPV4 server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (SocketAddress *)&server_address, sizeof(server_address)) != 0)
  {
    std::cerr << "Socket binding failed\n";
    return 1;
  }

  int request_queue_size = SOMAXCONN;
  if (listen(server_fd, request_queue_size) != 0)
  {
    std::cerr << "Listen failed\n";
    return 1;
  }

  std::vector<PollFD> poll_arguments;
  std::vector<Connection *> fd_to_connection;

  // Event Loop
  while (true)
  {
    poll_arguments.clear();

    PollFD server_poll_fd;
    server_poll_fd.events = POLLIN;
    server_poll_fd.fd = server_fd;
    server_poll_fd.revents = 0;

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
      poll_arguments.push_back(client_poll_fd);
    }

    int return_value = poll(poll_arguments.data(), (nfds_t)poll_arguments.size(), -1);
    if (return_value < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      else
      {
        return 1;
      }
    }

    if (poll_arguments[0].revents & POLLIN)
    {
      Connection *connection = handle_client_accept(server_fd);

      if (connection)
      { // Check if accept succeeded
        if (fd_to_connection.size() <= (size_t)connection->fd)
        {
          fd_to_connection.resize(connection->fd + 1);
        }
        fd_to_connection[connection->fd] = connection;
      }
    }

    for (size_t i = 1; i < poll_arguments.size(); ++i)
    {

      uint32_t ready = poll_arguments[i].revents;
      if (ready == 0)
      {
        continue;
      }

      int fd = poll_arguments[i].fd;
      Connection *connection = fd_to_connection[fd];

      // Safety check in case connection was deleted elsewhere
      if (!connection)
        continue;

      if ((ready & POLLIN) > 0)
      {
        handle_read(connection);
      }

      if ((ready & POLLOUT) > 0)
      {
        handle_write(connection);
      }

      if ((ready & POLLERR) || connection->want_close)
      {
        if (connection->fd != -1)
          close(connection->fd);
        fd_to_connection[connection->fd] = NULL;
        delete connection;
      }
    }
  }

  return 0;
}
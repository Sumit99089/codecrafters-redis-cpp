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
typedef std::vector<unsigned char>::const_iterator BufferedIterator;

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
static void buffer_append(
    const std::vector<unsigned char> &buffer,
    const unsigned char *data,
    unsigned long length);

static void buffer_consume(
    const std::vector<unsigned char> &buffer, // Changed to reference (&) so modifications persist
    unsigned long length);

static void handle_read(Connection *connection);
static void handle_write(Connection *connection);
static bool try_one_request(Connection *connection);
static int set_fd_nonblocking(int fd);
static Connection *handle_client_accept(int server_fd);

static int parse_header_value(
    BufferedIterator start,
    BufferedIterator end);

// ---------------------------------------------------------
// IMPLEMENTATION
// ---------------------------------------------------------

static void buffer_append(
    std::vector<unsigned char> &buffer,
    const unsigned char *data,
    unsigned long length)
{
  buffer.insert(buffer.end(), data, data + length);
}

static void buffer_consume(
    std::vector<unsigned char> &buffer,
    unsigned long length)
{
  buffer.erase(buffer.begin(), buffer.begin() + length);
}

static int parse_header_value(
    BufferedIterator start,
    BufferedIterator end)
{

  if (start == end)
  {
    return -2;
  }

  int value = 0;
  bool is_negative = false;

  BufferedIterator current = start;

  if (*current == '-')
  {
    is_negative = true;
    current++;
    if (current == end)
      return -2;
  }

  for (; current < end; ++current)
  {
    unsigned char c = *current;
    if (c < '0' || c > '9')
    {
      return -2;
    }

    value = (value * 10) + (c - '0');
  }

  return (is_negative) ? -value : value;
}

static void handle_read(Connection *connection)
{
  unsigned char buffer[1024 * 64] = {0};

  // Read data from the socket into the temporary buffer
  int bytes_read = read(
      connection->fd,
      buffer,
      sizeof(buffer));

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

  // Check for EOF (Client closed connection)
  if (bytes_read == 0)
  {
    if (connection->incoming_message.size() == 0)
    {
      std::cout << "Client Closed\n";
    }
    else
    {
      std::cout << "Unexpected End Of File\n";
    }
    connection->want_close = true;
    return;
  }

  // Append the data read from temporary buffer to Connection object's incoming message
  buffer_append(
      connection->incoming_message,
      buffer,
      bytes_read);

  // Keep on processing request until you exhaust them or encounter a partial request
  while (try_one_request(connection) == true)
  {
  }

  // Set write to true and read to false if there is any outgoing message
  if (connection->outgoing_message.size() > 0)
  {
    connection->want_read = false;
    connection->want_write = true;

    // Attempt to write immediately to avoid waiting for next poll cycle
    return handle_write(connection);
  }
}

static void handle_write(Connection *connection)
{
  // Send the data from the outgoing buffer to the client
  int sent_bytes = send(
      connection->fd,
      connection->outgoing_message.data(),
      connection->outgoing_message.size(),
      0);

  if (sent_bytes < 0)
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

  // Remove the bytes that were successfully sent from the outgoing message buffer
  buffer_consume(
      connection->outgoing_message,
      sent_bytes);

  // If outgoing message is empty, switch back to reading mode
  if (connection->outgoing_message.size() == 0)
  {
    connection->want_read = true;
    connection->want_write = false;
  }

  return;
}

static bool try_one_request(Connection *connection)
{
  size_t cursor = 0;
  // No incoming message. return. read = true
  if (connection->incoming_message.size() == 0)
  {
    return false;
  }
  // Message does not start with *. Does not follow RESP protocol. close = true
  if (connection->incoming_message[cursor] != '*')
  {
    std::cerr << "Protocol Error: Message must start with *\n";
    connection->want_read = false;
    connection->want_close = true;
    return false;
  }

  const char target[] = "\r\n";
  // Find the first carriage return(\r\n) after *. Then we can get the message length between * and \r\n.
  auto request_header_end_iterator = std::search(
      connection->incoming_message.begin(),
      connection->incoming_message.end(),
      target,
      target + 2);
  // Didnot find \r\n. So the entire header has not arrived. read = true. return
  if (request_header_end_iterator == connection->incoming_message.end())
  {
    return false;
  }

  int array_length = parse_header_value(
      connection->incoming_message.begin() + 1,
      request_header_end_iterator);

  std::vector<std::string> request_arguments;

  if (array_length < 0)
  {
    std::cerr << "Protocol Error: Invalid Array length\n";
    connection->want_read = false;
    connection->want_close = true;
    return false;
  }

  cursor = std::distance(connection->incoming_message.begin(), request_header_end_iterator) + 2; // Cursor at the next position of \r\n cursor->|
                                                                                                 //  [* , 1 , 2 , \r , \n, $ , .....]
  for (int i = 0; i < array_length; i++)
  {
    if (cursor >= connection->incoming_message.size())
    {
      return false; // partial request
    }
    if (connection->incoming_message[cursor] != '$')
    {
      std::cerr << "Protocol Error: Expected character $\n";
      connection->want_read = false;
      connection->want_close = true;
      return false;
    }
    // Find the first carriage return(\r\n) after $. Then we can get the message length between $ and \r\n.
    auto iterator = std::search(
        connection->incoming_message.begin() + cursor + 1,
        connection->incoming_message.end(),
        target,
        target + 2);
    // Didnot find \r\n. So the entire header has not arrived. read = true. return
    if (iterator == connection->incoming_message.end())
    {
      return false;
    }

    int string_length = parse_header_value(
        connection->incoming_message.begin() + cursor + 1,
        iterator);

    if (string_length < 0)
    {
      std::cerr << "Protocol Error: Invalid String Length\n";
      connection->want_read = false;
      connection->want_close = true;
      return false;
    }

    cursor = std::distance(connection->incoming_message.begin(), iterator) + 2; // Cursor at 3rd line

    if (cursor >= connection->incoming_message.size())
    {
      return false; // Parial request, read = true
    }

    // Cursor at the begining of string, set next_cursor at the end of the string.
    // next_cursor = cursor + string_length + 2(for \r\n)
    size_t next_cursor = cursor + string_length + 2;

    if (next_cursor > connection->incoming_message.size())
    {
      return false; // Parial request, read = true
    }

    std::string argument(
        connection->incoming_message.begin() + cursor,
        connection->incoming_message.begin() + cursor + string_length);

    request_arguments.push_back(argument);

    cursor = next_cursor;
  }

  if (request_arguments.size() > 0)
  {
    std::string command = request_arguments[0];

    if (command == "PING")
    {
      const char *response = "+PONG\r\n";
      buffer_append(
          connection->outgoing_message,
          (const unsigned char *)response,
          strlen(response));
    }
    else if (command == "ECHO")
    {

      if (request_arguments.size() > 1)
      {
        std::string payload = request_arguments[1];
        std::string response = "$" + std::to_string(payload.length()) + "\r\n" + payload + "\r\n";

        buffer_append(
            connection->outgoing_message,
            (const unsigned char *)response.c_str(),
            response.length());
      }
      else
      {
        // Send an error if the argument is missing
        const char *err = "-ERR wrong number of arguments for 'echo' command\r\n";
        buffer_append(connection->outgoing_message, (const unsigned char *)err, strlen(err));
      }
    }
    else
    {
      std::cerr << "Unknown Command\n";
      std::string err = "-ERR unknown command '" + command + "'\r\n";
      buffer_append(connection->outgoing_message, (const unsigned char *)err.c_str(), err.length());
    }
  }

  buffer_consume(connection->incoming_message, cursor);

  // Return true so the server loops again to check for pipelined requests
  return true;
}

static Connection *handle_client_accept(int server_fd)
{
  SocketAddressIPV4 client_address;
  SocketAddressSize client_address_length = sizeof(client_address);

  // Accept the new incoming connection
  int client_fd = accept(server_fd, (SocketAddress *)&client_address, &client_address_length);

  if (client_fd < 0)
  {
    return nullptr;
  }

  char *ip_string = inet_ntoa(client_address.sin_addr);
  uint16_t port = ntohs(client_address.sin_port);

  std::cout << "Client connected\n";
  std::cout << "IP:" << ip_string << "\n";
  std::cout << "PORT:" << port << "\n";

  // Set the new client socket to non-blocking mode
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
  // Update the file descriptor flags
  return fcntl(fd, F_SETFL, flags);
}

int main(int argc, char **argv)
{

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Create the server socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0)
  {
    std::cerr << "Failed to create socket server\n";
    return 1;
  }

  int reuse = 1;
  // Set socket option to reuse address to avoid 'Address already in use' errors
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "Set socket option to Reuse address failed\n";
    return 1;
  }

  // Set the server socket to non-blocking mode
  if (set_fd_nonblocking(server_fd) == -1)
  {
    std::cerr << "Failed to set server socket Non-Blocking\n";
    return 1;
  }

  SocketAddressIPV4 server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;

  // Bind the socket to the IP and Port
  if (bind(server_fd, (SocketAddress *)&server_address, sizeof(server_address)) != 0)
  {
    std::cerr << "Socket binding failed\n";
    return 1;
  }

  int request_queue_size = SOMAXCONN;
  // Start listening for incoming connections
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
        return 1;
      }
    }

    // Check if there is a new connection request on the server socket
    if (poll_arguments[0].revents & POLLIN)
    {
      Connection *connection = handle_client_accept(server_fd);

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
        handle_read(connection);
      }

      // Handle write event
      if ((ready & POLLOUT) > 0)
      {
        handle_write(connection);
      }

      // Handle errors or close request
      if ((ready & POLLERR) || connection->want_close)
      {
        if (connection->fd != -1)
          close(connection->fd);

        // Clear the connection from the map and free memory
        fd_to_connection[connection->fd] = NULL;
        delete connection;
      }
    }
  }

  return 0;
}
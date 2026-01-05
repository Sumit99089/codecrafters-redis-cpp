#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>



int main(int argc, char **argv)
{
  /*

  Flush after every std::cout / std::cerr.
  std::unitbuf: Makes the stream unbuffered, ie instead of storing the output in buffer until a newline occurs in case the output is going to terminal,
  it will be flushed immediately. In case of output redirected or piped to a file or another program, the behavior which is by default to store 4096 bytes of data or
  until the program exits.unitbuffer will make sure that the output is flushed immediately.

  Piping: ./a.out | another_program (a.out and another proigramvrunning parallelly, output of a.out is input to another_program)
  Redirection: ./a.out > output.txt (the output of a.out is written to output.txt file)

  */
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  /*
  socket(): Creates an endpoint for communication, can be bluetooth, wifi/ethernet(Internet for our case using AF_INET), between 2 processes etc.
  function signature: int socket(int domain, int type, int protocol);
  - AF_INET: "Address Family - Internet". Specifies we are using IPv4 (e.g., 127.0.0.1).
  - SOCK_STREAM: Specifies the transport type. We want a reliable, two-way connection stream (TCP).
                 (If we wanted UDP, we would use SOCK_DGRAM).
  - 0: The Protocol. Passing 0 tells the OS to choose the default protocol for the given type.
       For SOCK_STREAM, the default is TCP (IPPROTO_TCP).

  Return Value (server_fd): A "File Descriptor". In Linux, everything is a file. This integer is
  simply an index in the kernel's table pointing to this specific network socket resource.
  */
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  // Standard error checking: socket() returns -1 if it fails (e.g., if the OS runs out of file descriptors).
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  /*
  setsockopt(): Configures specific settings (options) for the socket.
  function signature: int setsockopt(int socket_fd, int level, int option_name, const void *option_value, socklen_t option_len);
  - SOL_SOCKET: "Socket Level". Says we are changing a setting at the socket layer itself (not the IP or TCP layer).
  - SO_REUSEADDR: The Option Name.
    Why do we need this?
    When a TCP server shuts down, the port (6379) often stays in a "TIME_WAIT" state for a minute or two
    to handle any straggling packets. If you try to restart your server immediately, the OS will say
    "Address already in use". Enabling REUSEADDR tells the OS: "I know this port is technically busy,
    but let me bind to it anyway."


    |Parameter,  |Type,       |Meaning,                                                                              |What you passed
    |sockfd,     |int,        |The Target. Which socket are we configuring?,                                         |server_fd
    |level,      |int,        |The Layer. Where does this setting live? (Socket level, IP level, or TCP level?),     |SOL_SOCKET (The generic socket layer)
    |optname,    |int,        |The Option. Which specific setting are we changing?,                                  |SO_REUSEADDR (Address Reuse)
    |optval,     |void*,      |The Value. Pointer to the new value (usually 1 for ""On"" or 0 for ""Off"").",        |&reuse (Pointer to integer 1)
    |optlen,     |socklen_t,  |The Size. How big is the value data?,                                                 |sizeof(reuse)
  */
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  /*
  struct sockaddr_in: A structure specifically designed to hold an IPv4 address and port.
  1. sin_family: Must match the socket type (AF_INET).
  2. sin_addr.s_addr = INADDR_ANY:
     This tells the server to accept connections on ANY available network interface.
     - If you used "127.0.0.1", it would only accept local connections.
     - INADDR_ANY allows connections from Wifi, Ethernet, Localhost, etc.
  3. sin_port = htons(6379):
     - htons means "Host TO Network Short".
     - Computers store numbers differently (Endianness). Your PC (x86) is likely "Little Endian"
       (stores least significant byte first). The Internet is "Big Endian".
     - This function flips the bytes of 6379 so the network understands the number correctly.


    struct sockaddr_in {
      sa_family_t    sin_family;  // 2 bytes: Address Family (AF_INET)
      in_port_t      sin_port;    // 2 bytes: Port Number (Must be network byte order!)
      struct in_addr sin_addr;    // 4 bytes: IP Address

      // Padding: This exists to make the struct the same size as "sockaddr"
      unsigned char  sin_zero[8];
    };

    struct in_addr {
      uint32_t s_addr; // 32-bit Integer (0.0.0.0 to 255.255.255.255)
    };

    struct sockaddr {
      sa_family_t sa_family;      // 2 bytes: Address Family
      char        sa_data[14];    // 14 bytes: Protocol Address (IP + Port mixed together)
    };
  */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  /*
  bind(): The "Glue".
  Right now, 'server_fd' is just a socket, and 'server_addr' is just a piece of paper with an address written on it.
  They are not connected.
  bind() forcibly attaches the address (IP 0.0.0.0, Port 6379) to the socket (server_fd).
  After this, any traffic arriving at Port 6379 belongs to this socket.

  When you run int fd = socket(...), the Operating System allocates a specific block of memory in the Kernel to manage this connection.

  Ques. What can you do with it?
  Ans.  At this specific moment (Post-Socket, Pre-Bind), the socket is in a "Limbo" state.

      1.Can you listen? No. You have no address.
      2.Can you accept clients? No. They can't find you.
      3.Can you connect to others? YES.
            Surprise: If you use this socket to dial out (connect to https://www.google.com/search?q=Google.com),
            the OS will automatically assign a random ephemeral port (like 54321) to it just for that call.

  Ques. Why didn't they make a function create_socket_at_address(IP, Port)?
  Ans.  Because sometimes you don't want to bind.

      1. Client Mode: If you are writing a Client (like a web browser), you don't care what your local port is. You just create a socket()
      and immediately connect(). The OS picks a random port for you.

      2. Server Mode: If you are writing a Server, you care deeply about the port. So you take the extra step to bind() it to a specific
      number (like 6379) so users can find you consistently.

  Return Value: 0 on success, -1 on failure (e.g., if the port is already in use).
  */
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }

  /*
  listen(): Marks the socket as "Passive".
  Before this call, the socket could technically be used to make outgoing calls.
  listen() tells the OS: "I am not calling anyone. I am waiting for others to call me."

  - connection_backlog (5): The size of the "Pending Queue".
    If 5 clients try to connect at the EXACT same millisecond, the OS will hold 5 of them in a queue
    until your code calls accept(). The 6th one might be rejected.
  */


  /*
  Client                               Server Kernel                            Server Application
      --------                             ---------------                          --------------------
         |                                        |                                           |
    1. sends SYN  ---------------------------> [ SYN Queue ]                                  |
         |                                        |                                           |
         | <--------------------------- sends SYN/ACK                                         |
         |                                        |                                           |
    2. sends ACK  ---------------------------> [ Check Backlog Size ]                         |
         |                                        |                                           |
         |                                  Is Accept Queue full?                             |
         |                                      /       \                                     |
         |                                    NO         YES                                  |
         |                                     |          |                                   |
    Connection is                     [ Move to      [ Drop Packet ]                          |
    ESTABLISHED                     Accept Queue ]   (Client retries)                         |
                                           |                                                  |
                                           | <--------------------------------------- calls accept()
                                           |                                                  |
                                    [ Remove from Queue ] --------------------------> Returns new FD
  */
  int connection_backlog = 5; // the queue mentioned above is what we are trying to set size of
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  } 

  // Just setting up variables to hold the details of whoever connects to us.
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";
  std::cout << "Logs from your program will appear here!\n";

  /*
  accept(): The "Gatekeeper".
  1. This function BLOCKS (pauses your program completely).
  2. It waits for the TCP "3-Way Handshake" (SYN, SYN-ACK, ACK) to complete with a client.
  3. Once a client connects, accept() returns a BRAND NEW file descriptor (client_fd).

  CRITICAL CONCEPT:
  - server_fd: Stays listening at the door forever.
  - client_fd: Is a private line specifically for THIS one client.
  */
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
  std::cout << "Client connected\n";

  // Create a buffer (array) of 1024 bytes, initialized to all zeros (null terminators/ \0).
  char buffer[1024] = {0};

  while (true)
  {
    /*
    read():
    - Reads data from the network buffer (kernel space) into your 'buffer' array (user space).
    - It BLOCKS until at least 1 byte is available.
    - Returns: The number of bytes actually read.
    */
    int bytes_read = read(client_fd, buffer, sizeof(buffer));

    // If read returns negative, it's a system error (e.g., connection reset).
    if (bytes_read < 0)
    {
      std::cerr << "Failed to read client message\n";
      break;
    }

    // NOTE: You are missing a check for "bytes_read == 0", which signifies the client disconnected safely.
    // Without it, if the client quits, this loop might spin infinitely reading 0 bytes.
    else if( bytes_read == 0){
      std::cerr<< "The client has disconnected\n";
      break;
    }

    /*ipaddr
    std::string request(buffer):
    Constructs a C++ string object from the C-style char array.
    It copies characters from 'buffer' until it hits a null terminator '\0'.
    */
    std::string request(buffer);

    /*
    find(): Searches the string for "PING".
    npos: "No Position". A constant meaning "I couldn't find it".
    So, "!= npos" means "I found it!"
    */
    if (request.find("PING") != std::string::npos)
    {
      const char *response = "+PONG\r\n";

      /*
      send():
      - Pushes your response data back to the kernel to be transmitted over the network.
      - client_fd: The specific client to talk to.
      - response: The data.
      - strlen: How many bytes to send.
      - 0: Flags (0 makes it standard, similar to write()).
      */
      int result = send(client_fd, response, strlen(response), 0); //blocking call
      std::cout<<strlen(response)<<" bytes sent to client\n";

      if(result == -1){
        std::cerr<<"Error writing to client\n";
      }
    }
  }
  close(client_fd);

  return 0;
}

/*
Post-Increment (i++):

Logic: "Make a copy of the old value. Increment the real value. Return the copy."

Cost: Creates a temporary copy (wasted memory/cycles if you don't use it).

Pre-Increment (++i):

Logic: "Increment the real value. Return the new value."

Cost: No temporary copy. It is direct.

*/


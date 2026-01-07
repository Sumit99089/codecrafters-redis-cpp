#include "Server.hpp"
#include <iostream>

int main(int argc, char **argv) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    Server server(6379);
    server.run();

    return 0;
}
#pragma once
#include <vector>
#include <string>
#include "KeyValueStore.hpp"

class Connection
{
public:
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;
    KeyValueStore &kv_store;

    // We use your existing buffer types
    std::vector<unsigned char> incoming_message;
    std::vector<unsigned char> outgoing_message;

    Connection(int fd, KeyValueStore &store); // Constructor
    ~Connection();                            // Destructor

    void handle_read();
    void handle_write();

private:
    // Helper functions specific to a single connection
    bool try_one_request();
    void buffer_append(std::vector<unsigned char> &buffer, const unsigned char *data, unsigned long length);
    void buffer_consume(std::vector<unsigned char> &buffer, unsigned long length);
};
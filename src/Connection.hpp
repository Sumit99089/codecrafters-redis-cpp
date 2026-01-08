#pragma once
#include <vector>
#include <string>
#include "KeyValueStore.hpp"

//======================  SECURITY LIMITS START  ======================

// 1. Max size of a single Bulk String (16MB).
const size_t MAX_MSG_SIZE = 16 * 1024 * 1024;

// 2. Max size of the entire buffer.
// MUST be larger than MAX_MSG_SIZE to account for protocol headers (*3\r\n$5...)
// We add 1 MB of headroom just to be safe.
const size_t MAX_REQUEST_SIZE = MAX_MSG_SIZE + (1024 * 1024); // 17MB

// 3. Max number of arguments (1024).
const int MAX_ARGS_COUNT = 1024;

//======================   SECURITY LIMITS END   ======================

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
    template <typename T>
    long long parse_header_value(T start, T end)
    {
        if (start == end)
            return -2;

        long long value = 0;
        bool is_negative = false;
        T current = start;

        if (*current == '-')
        {
            is_negative = true;
            current++;
            if (current == end)
                return -2;
        }

        for (; current < end; ++current)
        {
            // This cast ensures 'char' or 'unsigned char' works the same
            unsigned char c = static_cast<unsigned char>(*current);
            if (c < '0' || c > '9')
                return -2;
            value = (value * 10) + (c - '0');
        }

        return (is_negative) ? -value : value;
    }
};
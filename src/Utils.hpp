#pragma once
#include <fcntl.h>
#include <unistd.h>

// Sets a file descriptor to non-blocking mode
inline int set_fd_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

// Universal parser for numeric headers
template <typename T>
inline long long parse_header_value(T start, T end)
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

    // Using != instead of < makes this compatible with more iterator types
    for (; current != end; ++current)
    {
        unsigned char c = static_cast<unsigned char>(*current);
        if (c < '0' || c > '9')
            return -2;
        value = (value * 10) + (c - '0');
    }

    return (is_negative) ? -value : value;
}
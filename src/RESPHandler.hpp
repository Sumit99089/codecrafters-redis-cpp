#pragma once
#include <vector>
#include <string>
#include <optional>

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

enum class ParseStatus
{
    SUCCESS,
    PARTIAL,
    ERROR,
};

struct RESPRequest
{
    ParseStatus status;
    std::vector<std::string> args;
    size_t parsed_bytes;
};

class RESPHandler
{
public:
    // Parses the buffer and returns arguments + number of bytes consumed
    static RESPRequest parse_request(const std::vector<unsigned char> &buffer);

    // Serialization helpers to wrap responses back into RESP
    static std::string serialize_simple_string(const std::string &s);
    static std::string serialize_error(const std::string &e);
    static std::string serialize_bulk_string(const std::string &s);
    static std::string serialize_null_bulk();
    static std::string serialize_integer(long long val); 
};
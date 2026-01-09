#include "RESPHandler.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <iostream>

RESPRequest RESPHandler::parse_request(const std::vector<unsigned char> &buffer)
{

    size_t cursor = 0;
    RESPRequest resp_req;
    // No incoming message. return. read = true
    if (buffer.size() == 0)
    {
        resp_req.status = ParseStatus::PARTIAL;
        return resp_req;
    }
    // Message does not start with *. Does not follow RESP protocol. close = true
    if (buffer[cursor] != '*')
    {
        std::cerr << "Protocol Error: Message must start with *\n";
        resp_req.status = ParseStatus::ERROR;
        return resp_req;
    }

    const char target[] = "\r\n";

    // Find the first carriage return(\r\n) after *. Then we can get the message length between * and \r\n.
    auto request_header_end_iterator = std::search(
        buffer.begin(),
        buffer.end(),
        target,
        target + 2);

    // Did not find \r\n. So the entire header has not arrived. read = true. return
    if (request_header_end_iterator == buffer.end())
    {
        resp_req.status = ParseStatus::PARTIAL;
        return resp_req;
    }

    long long array_length = parse_header_value(
        buffer.begin() + 1,
        request_header_end_iterator);

    if (array_length < 0)
    {
        std::cerr << "Protocol Error: Invalid Array Length\n";
        resp_req.status = ParseStatus::ERROR;
        return resp_req;
    }

    // We use static_cast<size_t> to tell the compiler: "I know this int is positive now, so treat it as unsigned."
    if (static_cast<size_t>(array_length) > MAX_ARGS_COUNT)
    {
        std::cerr << "Security: Array too large\n";
        resp_req.status = ParseStatus::ERROR;
        return resp_req;
    }

    std::vector<std::string> request_arguments;

    cursor = std::distance(buffer.begin(), request_header_end_iterator) + 2;
    // Cursor at the next position of \r\n. cursor->|
    //                        [* , 1 , 2 , \r , \n, $ , .....]
    for (int i = 0; i < array_length; i++)
    {
        if (cursor >= buffer.size())
        {
            resp_req.status = ParseStatus::PARTIAL;
            return resp_req;
        }

        if (buffer[cursor] != '$')
        {
            std::cerr << "Protocol Error: Expected character $\n";
            resp_req.status = ParseStatus::ERROR;
            return resp_req;
        }
        // Find the first carriage return(\r\n) after $. Then we can get the message length between $ and \r\n.
        auto iterator = std::search(
            buffer.begin() + cursor + 1,
            buffer.end(),
            target,
            target + 2);
        // Didnot find \r\n. So the entire header has not arrived. read = true. return
        if (iterator == buffer.end())
        {
            resp_req.status = ParseStatus::PARTIAL;
            return resp_req;
        }

        long long string_length = parse_header_value(
            buffer.begin() + cursor + 1,
            iterator);

        if (string_length < 0)
        {
            std::cerr << "Protocol Error: Invalid String Length\n";
            resp_req.status = ParseStatus::ERROR;
            return resp_req;
        }

        // 2. Second, check for Security Limits (Size)
        // We use static_cast<size_t> to tell the compiler: "I know this int is positive now, so treat it as unsigned."
        if (static_cast<size_t>(string_length) > MAX_MSG_SIZE)
        {
            std::cerr << "Security: String too large\n"; // Distinct error message!
            resp_req.status = ParseStatus::ERROR;
            return resp_req;
        }

        cursor = std::distance(buffer.begin(), iterator) + 2; // Cursor at 3rd line

        if (cursor >= buffer.size())
        {
            resp_req.status = ParseStatus::PARTIAL;
            return resp_req; // Parial request, read = true
        }

        // Cursor at the begining of string, set next_cursor at the end of the string.
        // next_cursor = cursor + string_length + 2(for \r\n)
        size_t next_cursor = cursor + string_length + 2;

        if (next_cursor > buffer.size())
        {
            resp_req.status = ParseStatus::PARTIAL;
            return resp_req; // Parial request, read = true
        }

        std::string argument(
            buffer.begin() + cursor,
            buffer.begin() + cursor + string_length);

        request_arguments.push_back(argument);

        cursor = next_cursor;
    }

    resp_req.status = ParseStatus::SUCCESS;
    resp_req.args = std::move(request_arguments);
    resp_req.parsed_bytes = cursor;
    return resp_req;
}

std::string RESPHandler::serialize_simple_string(const std::string &s)
{
    return "+" + s + "\r\n";
}
std::string RESPHandler::serialize_error(const std::string &e)
{
    return "-" + e + "\r\n";
}
std::string RESPHandler::serialize_bulk_string(const std::string &s)
{
    std::string len_str = std::to_string(s.length());
    std::string result;
    // Calculate total size: 1 ($) + length string + 2 (\r\n) + data + 2 (\r\n)
    result.reserve(1 + len_str.length() + 2 + s.length() + 2);

    result += '$';
    result += len_str;
    result += "\r\n";
    result += s;
    result += "\r\n";
    return result;
}

std::string RESPHandler::serialize_null_bulk()
{
    return "$-1\r\n";
}
std::string RESPHandler::serialize_integer(long long val)
{
}
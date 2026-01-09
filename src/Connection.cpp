#include "KeyValueStore.hpp"
#include "Connection.hpp"
#include "RESPHandler.hpp"
#include "Utils.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sys/socket.h>

// Constructor Definition
Connection::Connection(int fd, KeyValueStore &store) : kv_store(store)
{
    this->fd = fd;
    this->want_read = true;
}

// Destructor Definiton
Connection::~Connection()
{
    if (fd != -1)
    {
        close(fd);
    }
}

void Connection::handle_read()
{
    unsigned char buffer[1024 * 64] = {0};

    // Read data from the socket into the temporary buffer
    int bytes_read = read(
        this->fd,
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
            this->want_close = true;
            return;
        }
    }

    // Check for EOF (Client closed connection)
    if (bytes_read == 0)
    {
        if (this->incoming_message.size() == 0)
        {
            std::cout << "Client Closed\n";
        }
        else
        {
            std::cout << "Unexpected End Of File\n";
        }
        this->want_close = true;
        return;
    }

    if (bytes_read + this->incoming_message.size() > MAX_REQUEST_SIZE)
    {
        this->want_read = false;
        this->want_close = true;
        return;
    }

    // Append the data read from temporary buffer to Connection object's incoming message
    buffer_append(
        this->incoming_message,
        buffer,
        bytes_read);

    // Keep on processing request until you exhaust them or encounter a partial request
    while (try_one_request() == true)
    {
    }

    // Set write to true and read to false if there is any outgoing message
    if (this->outgoing_message.size() > 0)
    {
        this->want_read = false;
        this->want_write = true;

        // Attempt to write immediately to avoid waiting for next poll cycle
        return handle_write();
    }
}

void Connection::handle_write()
{
    // Send the data from the outgoing buffer to the client
    int sent_bytes = send(
        this->fd,
        this->outgoing_message.data(),
        this->outgoing_message.size(),
        0);

    if (sent_bytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        else
        {
            this->want_close = true;
            return;
        }
    }

    // Remove the bytes that were successfully sent from the outgoing message buffer
    buffer_consume(
        this->outgoing_message,
        sent_bytes);

    // If outgoing message is empty, switch back to reading mode
    if (this->outgoing_message.size() == 0)
    {
        this->want_read = true;
        this->want_write = false;
    }

    return;
}

bool Connection::try_one_request()
{
    RESPRequest request = RESPHandler::parse_request(this->incoming_message);

    if(request.status == ParseStatus::ERROR){
        this->want_read = false;
        this->want_close = true;
        return false;
    }
    else if( request.status == ParseStatus::PARTIAL){
        return false;
    }

    if (request.args.size() > 0)
    {
        std::string command = request.args[0];

        if (command == "PING")
        {
            const char *response = "+PONG\r\n";
            buffer_append(
                this->outgoing_message,
                (const unsigned char *)response,
                strlen(response));
        }
        else if (command == "ECHO")
        {
            if (request.args.size() > 1)
            {
                std::string payload = request.args[1];
                std::string response = "$" + std::to_string(payload.length()) + "\r\n" + payload + "\r\n";

                buffer_append(
                    this->outgoing_message,
                    (const unsigned char *)response.c_str(),
                    response.length());
            }
            else
            {
                // Send an error if the argument is missing
                const char *err = "-ERR wrong number of arguments for 'echo' command\r\n";
                buffer_append(this->outgoing_message, (const unsigned char *)err, strlen(err));
            }
        }
        else if (command == "GET")
        {

            if (request.args.size() > 1)
            {
                std::string &key = request.args[1];
                std::optional<KeyValueStore::ValueEntry> result = this->kv_store.get(key);
                if (result.has_value() == true)
                {
                    std::string value = result->value;
                    std::optional<std::chrono::steady_clock::time_point> expires_at = result->expires_at;
                    std::string response;
                    if (expires_at <= std::chrono::steady_clock::now())
                    {
                        response = "$-1\r\n";
                    }
                    else
                    {
                        response = "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
                    }

                    buffer_append(
                        this->outgoing_message,
                        (const unsigned char *)response.c_str(),
                        response.length());
                }
                else
                {
                    const char *null_response = "$-1\r\n";
                    buffer_append(
                        this->outgoing_message,
                        (const unsigned char *)null_response,
                        strlen(null_response));
                }
            }
            else
            {
                const char *err = "-ERR wrong number of arguments for 'get' command\r\n";
                buffer_append(this->outgoing_message, (const unsigned char *)err, strlen(err));
            }
        }
        else if (command == "SET")
        {
            if (request.args.size() < 3)
            {
                const char *err = "-ERR wrong number of arguments for 'set' command\r\n";
                buffer_append(this->outgoing_message, (const unsigned char *)err, strlen(err));
            }
            else
            {
                std::string key = request.args[1];
                std::string value = request.args[2];
                std::optional<std::chrono::steady_clock::time_point> expiry = std::nullopt;
                std::string condition = "";
                bool keep_ttl = false;
                bool want_get = false;

                for (size_t i = 3; i < request.args.size(); i++)
                {
                    std::string argument = request.args[i];

                    if (argument == "NX")
                    {
                        if (!condition.empty())
                            goto syntax_error; // Already had NX or XX
                        condition = "NX";
                    }
                    else if (argument == "XX")
                    {
                        if (!condition.empty())
                            goto syntax_error; // Already had NX or XX
                        condition = "XX";
                    }
                    else if (argument == "GET")
                    {
                        if (want_get)
                            goto syntax_error; // Duplicate GET
                        want_get = true;
                    }
                    else if (argument == "KEEPTTL")
                    {
                        if (expiry != std::nullopt || keep_ttl == true)
                            goto syntax_error;
                        keep_ttl = true;
                    }
                    else if (argument == "PX" || argument == "EX")
                    {
                        if (keep_ttl || expiry != std::nullopt)
                            goto syntax_error;

                        if (i + 1 < request.args.size())
                        {
                            std::string val_str = request.args[++i];
                            long long time_val = parse_header_value(val_str.begin(), val_str.end());

                            if (time_val <= 0)
                            {
                                std::string err = "-ERR value is not an integer or out of range\r\n";
                                buffer_append(this->outgoing_message, (const unsigned char *)err.c_str(), err.length());
                                return true;
                            }

                            auto time_now = std::chrono::steady_clock::now();
                            if (argument == "PX")
                                expiry = time_now + std::chrono::milliseconds(time_val);
                            else
                                expiry = time_now + std::chrono::seconds(time_val);
                        }
                        else
                        {
                            goto syntax_error;
                        } // No number provided after PX/EX
                    }
                    else
                    {
                        // Truly an unknown flag for the SET command
                        goto syntax_error;
                    }
                }
            syntax_error:
            {
                const char *err = "-ERR syntax error\r\n";
                buffer_append(this->outgoing_message, (const unsigned char *)err, strlen(err));
                return true;
            }

                KeyValueStore::ValueEntry value_entry = {value, expiry};
                this->kv_store.set(key, value_entry);

                // After the loop, execute the KVStore logic...
                return true;
            }
        }
        else
        {
            std::cerr << "Unknown Command\n";
            std::string err = "-ERR unknown command '" + command + "'\r\n";
            buffer_append(this->outgoing_message, (const unsigned char *)err.c_str(), err.length());
        }
    }

    buffer_consume(this->incoming_message, request.parsed_bytes);

    // Return true so the server loops again to check for pipelined requests
    return true;
}

void Connection::buffer_append(std::vector<unsigned char> &buffer, const unsigned char *data, unsigned long length)
{
    buffer.insert(buffer.end(), data, data + length);
}

void Connection::buffer_consume(std::vector<unsigned char> &buffer, unsigned long length)
{
    buffer.erase(buffer.begin(), buffer.begin() + length);
}

#include "CommandDispatcher.hpp"

std::string CommandDispatcher::dispatch(const std::vector<std::string> &args, KeyValueStore &store)
{
    if (args.empty())
        return "";
    std::string command = args[0];

    if (command == "PING")
        return handle_ping(args);
    if (command == "GET")
        return handle_get(args, store);
    if (command == "SET")
        return handle_set(args, store);

    return RESPHandler::serialize_error("unknown command");
}

std::string CommandDispatcher::handle_ping(const std::vector<std::string> &args)
{
    std::string response = "PONG";
    return response;
}
std::string CommandDispatcher::handle_get(const std::vector<std::string> &args, KeyValueStore &store)
{
    
}
std::string CommandDispatcher::handle_set(const std::vector<std::string> &args, KeyValueStore &store)
{
}

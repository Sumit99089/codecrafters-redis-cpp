#pragma once
#include "RESPHandler.hpp"
#include "KeyValueStore.hpp"
#include<string>
#include<vector>


class CommandDispatcher {
public:
    std::string dispatch(const std::vector<std::string>& args, KeyValueStore& store) {
        if (args.empty()) return "";
        std::string command = args[0];
        
        if (command == "PING") return handle_ping(args);
        if (command == "GET")  return handle_get(args, store);
        if (command == "SET")  return handle_set(args, store);
        
        return RESPHandler::serialize_error("unknown command");
    }

private:
    std::string handle_ping(const std::vector<std::string>& args);
    std::string handle_get(const std::vector<std::string>& args, KeyValueStore& store);
    std::string handle_set(const std::vector<std::string>& args, KeyValueStore& store);
};
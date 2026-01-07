#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>

class KeyValueStore {
public:
    KeyValueStore() = default;

    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(store_mutex);
        data[key] = value;
    }

    std::optional<std::string> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(store_mutex);
        auto it = data.find(key);
        if (it != data.end()) {
            return it->second;
        }
        return std::nullopt;
    }   

private:
    // The actual "Town Square" where data lives
    std::unordered_map<std::string, std::string> data;
    
    // Mutex to ensure thread safety
    std::mutex store_mutex;
};
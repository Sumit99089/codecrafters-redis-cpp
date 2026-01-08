#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <chrono>

typedef struct ValueEntry Entry;

class KeyValueStore
{
public:
    KeyValueStore() = default;

    struct ValueEntry
    {
        std::string value;
        std::optional<std::chrono::steady_clock::time_point> expires_at;
    };

    void set(const std::string &key, const ValueEntry &value)
    {
        std::lock_guard<std::mutex> lock(store_mutex);
        data[key] = value;
    }

    std::optional<ValueEntry> get(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(store_mutex);
        auto it = data.find(key);
        if (it != data.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

private:
    // The actual "Town Square" where data lives
    std::unordered_map<std::string, ValueEntry> data;

    // Mutex to ensure thread safety
    std::mutex store_mutex;
};
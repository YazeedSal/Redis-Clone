#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <mutex>
#include <vector>

class Store {
public:
    // SET key value
    // Stores the value under the key.
    // If the key already exists, overwrites it.
    void set(const std::string& key, const std::string& value);

    // GET key
    // Returns the value if the key exists and hasn't expired.
    // Returns std::nullopt if the key doesn't exist or has expired.
    std::optional<std::string> get(const std::string& key);

    // DEL key
    // Deletes the key and its expiry if it exists.
    // Returns 1 if the key existed, 0 if it didn't.
    int del(const std::string& key);

    // EXISTS key
    // Returns 1 if the key exists and hasn't expired, 0 otherwise.
    int exists(const std::string& key);

    // EXPIRE key seconds
    // Sets a TTL on the key — it will expire after `seconds` seconds.
    // Returns 1 if the key exists and the TTL was set, 0 otherwise.
    int expire(const std::string& key, int seconds);

    // TTL key
    // Returns the remaining time to live in seconds.
    // Returns -1 if the key exists but has no expiry.
    // Returns -2 if the key doesn't exist or has expired.
    int ttl(const std::string& key);

    // KEYS pattern
    // Returns all keys matching the given pattern.
    // Supports * (any sequence) and ? (any single character) wildcards.
    std::vector<std::string> keys(const std::string& pattern);

    // INCR key
    // Increments the integer value of a key by 1.
    // If the key doesn't exist, starts from 0.
    // Returns the new value, or an error string if the value isn't an integer.
    // We use a struct to return either a value or an error cleanly.
    struct IncrResult {
    	bool ok;
    	int value;
    	std::string error;
    };

    IncrResult incr(const std::string& key);
    IncrResult decr(const std::string& key);

    // APPEND key value
    // Appends value to the end of the string stored at key.
    // If key doesn't exist, creates it with value (like SET).
    // Returns the new length of the string after appending.
    int append(const std::string& key, const std::string& value);

    // RENAME key newkey
    // Renames key to newkey.
    // If newkey already exists, it is overwritten.
    // If key has a TTL, it carries over to newkey.
    // Returns true if successful, false if key doesn't exist.
    bool rename(const std::string& key, const std::string& newkey);

private:
    // The main key-value store
    std::unordered_map<std::string, std::string> data_;

    // Stores expiry times for keys that have a TTL.
    // Not all keys have an entry here — only those with EXPIRE set.
    std::unordered_map<std::string,
        std::chrono::steady_clock::time_point> expiry_;

    // Protects both data_ and expiry_ from concurrent access.
    // Mutable because we need to lock it in const contexts.
    mutable std::mutex mutex_;

    // Checks if a key has expired.
    // If it has, removes it from both maps and returns true.
    // IMPORTANT: must be called with mutex_ already locked.
    bool is_expired(const std::string& key);
};

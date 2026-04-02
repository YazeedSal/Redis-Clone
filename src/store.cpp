#include "store.h"

// is_expired() checks if a key has a TTL and whether it has passed.
// If expired, it cleans up both maps and returns true.
// CALLER MUST HOLD mutex_ before calling this.
bool Store::is_expired(const std::string& key) {
    // Check if this key has an expiry set
    auto it = expiry_.find(key);
    if (it == expiry_.end()) {
        // No expiry set — key lives forever
        return false;
    }

    // Check if the expiry time has passed
    auto now = std::chrono::steady_clock::now();
    if (now >= it->second) {
        // Key has expired — remove it from both maps
        data_.erase(key);
        expiry_.erase(it);
        return true;
    }

    return false;
}

void Store::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    data_[key] = value;

    // Setting a key clears any existing expiry.
    // This matches real Redis behavior — SET resets the TTL.
    expiry_.erase(key);
}

std::optional<std::string> Store::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check expiry first — lazy expiry happens here
    if (is_expired(key)) {
        return std::nullopt;
    }

    auto it = data_.find(key);
    if (it == data_.end()) {
        return std::nullopt;
    }

    return it->second;
}

int Store::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // erase() returns the number of elements removed (0 or 1)
    int removed = data_.erase(key);

    // Clean up expiry entry too if it exists
    expiry_.erase(key);

    return removed;
}

int Store::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_expired(key)) {
        return 0;
    }

    return data_.count(key);
}

int Store::expire(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Can't set expiry on a key that doesn't exist
    if (data_.find(key) == data_.end()) {
        return 0;
    }

    // Calculate the absolute expiry time point
    // now() + duration gives us a point in the future
    expiry_[key] = std::chrono::steady_clock::now() +
                   std::chrono::seconds(seconds);

    return 1;
}

int Store::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Key doesn't exist or has expired
    if (is_expired(key) || data_.find(key) == data_.end()) {
        return -2;
    }

    // Key exists but has no expiry
    auto it = expiry_.find(key);
    if (it == expiry_.end()) {
        return -1;
    }

    // Calculate remaining time
    auto now = std::chrono::steady_clock::now();
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        it->second - now
    );

    return static_cast<int>(remaining.count());
}

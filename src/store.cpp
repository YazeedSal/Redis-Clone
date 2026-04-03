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




// Pattern matching helper.
// Returns true if `str` matches `pattern`.
// Supports * and ? wildcards.
// This is a private file-local function — nothing outside needs it.
static bool match_pattern(const std::string& pattern, size_t p,
                           const std::string& str, size_t s) {
    // Base case — reached the end of the pattern
    if (p == pattern.size()) {
        // Match only if we also reached the end of the string
        return s == str.size();
    }

    if (pattern[p] == '*') {
        // '*' can match zero characters — skip the '*' and try
        // OR match one character — advance s and try again with same '*'
        return match_pattern(pattern, p + 1, str, s) ||
               (s < str.size() && match_pattern(pattern, p, str, s + 1));
    }

    if (pattern[p] == '?') {
        // '?' matches exactly one character — both must advance
        return s < str.size() &&
               match_pattern(pattern, p + 1, str, s + 1);
    }

    // Regular character — must match exactly
    return s < str.size() &&
           pattern[p] == str[s] &&
           match_pattern(pattern, p + 1, str, s + 1);
}

std::vector<std::string> Store::keys(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> result;

    for (auto& [key, value] : data_) {
        // Skip expired keys — check expiry without calling is_expired()
        // directly since that modifies the map while we're iterating.
        // Instead we check manually and collect expired keys separately.
        auto exp_it = expiry_.find(key);
        if (exp_it != expiry_.end()) {
            if (std::chrono::steady_clock::now() >= exp_it->second) {
                continue; // expired — skip it
            }
        }

        // Check if this key matches the pattern
        if (match_pattern(pattern, 0, key, 0)) {
            result.push_back(key);
        }
    }

    return result;
}

// Shared helper for INCR and DECR.
// Reads the current value, interprets it as an integer,
// applies the delta (+1 or -1), stores the result, and returns it.
static Store::IncrResult incr_by(
    std::unordered_map<std::string, std::string>& data,
    const std::string& key,
    int delta)
{
    // If key doesn't exist, treat current value as 0
    int current = 0;

    auto it = data.find(key);
    if (it != data.end()) {
        // Key exists — try to parse its value as an integer
        try {
            size_t pos = 0;
            current = std::stoi(it->second, &pos);

            // Make sure the entire string was consumed —
            // "42abc" would parse as 42 but is not a valid integer value
            if (pos != it->second.size()) {
                return {false, 0, "value is not an integer or out of range"};
            }
        } catch (...) {
            return {false, 0, "value is not an integer or out of range"};
        }
    }

    // Apply the delta and store the result back as a string
    int new_value = current + delta;
    data[key] = std::to_string(new_value);

    return {true, new_value, ""};
}

Store::IncrResult Store::incr(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Handle expiry first
    if (is_expired(key)) {
        // Expired keys are treated as non-existent — start from 0
        expiry_.erase(key);
    }

    return incr_by(data_, key, +1);
}

Store::IncrResult Store::decr(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_expired(key)) {
        expiry_.erase(key);
    }

    return incr_by(data_, key, -1);
}

int Store::append(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    // If key has expired, treat it as non-existent
    if (is_expired(key)) {
        expiry_.erase(key);
    }

    // operator+= on std::string appends in place.
    // If key doesn't exist, data_[key] is default-constructed
    // as an empty string, so appending to it is the same as SET.
    data_[key] += value;

    return static_cast<int>(data_[key].size());
}

bool Store::rename(const std::string& key, const std::string& newkey) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Can't rename an expired or non-existent key
    if (is_expired(key) || data_.find(key) == data_.end()) {
        return false;
    }

    // If key and newkey are the same, nothing to do
    if (key == newkey) {
        return true;
    }

    // Move the value to the new key
    data_[newkey] = std::move(data_[key]);
    data_.erase(key);

    // Move the TTL over to the new key if one exists.
    // If newkey already had a TTL, it gets overwritten — correct behavior.
    auto it = expiry_.find(key);
    if (it != expiry_.end()) {
        expiry_[newkey] = it->second;
        expiry_.erase(it);
    } else {
        // newkey might have had its own TTL — clear it
        // since the source key had no expiry
        expiry_.erase(newkey);
    }

    return true;
}


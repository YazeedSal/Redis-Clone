#include "resp_parser.h"

#include <stdexcept>
#include <string>

// A quick helper to read one full line ending in \r\n.
// Returns the line content WITHOUT the \r\n.
// Advances `pos` to point to the character after the \r\n.
static std::string read_line(const std::string& raw, size_t& pos) {
    // Find the \r\n starting from the current position
    size_t end = raw.find("\r\n", pos);

    if (end == std::string::npos) {
        throw std::runtime_error("RESP parse error: expected \\r\\n not found");
    }

    // Extract the line content (everything between pos and the \r\n)
    std::string line = raw.substr(pos, end - pos);

    // Move pos past the \r\n so the next read starts fresh
    pos = end + 2;

    return line;
}

std::string RespParser::parse_bulk_string(const std::string& raw, size_t& pos) {
    // A bulk string starts with $<length>\r\n
    // Example: $5\r\nHello\r\n
    std::string header = read_line(raw, pos);

    // The first character must be '$'
    if (header.empty() || header[0] != '$') {
        throw std::runtime_error("RESP parse error: expected '$', got: " + header);
    }

    // Parse the length from the header (everything after the '$')
    // std::stoi converts a string like "5" to the integer 5
    int length = std::stoi(header.substr(1));

    if (length < 0) {
        throw std::runtime_error("RESP parse error: negative bulk string length");
    }

    // Now read exactly `length` bytes — that's the actual string content
    std::string value = raw.substr(pos, length);

    // Advance pos past the content AND the trailing \r\n
    pos += length + 2;

    return value;
}

Command RespParser::parse(const std::string& raw) {
    // All Redis commands arrive as RESP arrays.
    // An array starts with *<count>\r\n
    // Example: *3\r\n means "this array has 3 elements"

    size_t pos = 0;

    // Read the first line — it must start with '*'
    std::string header = read_line(raw, pos);

    if (header.empty() || header[0] != '*') {
        throw std::runtime_error("RESP parse error: expected '*', got: " + header);
    }

    // Parse how many elements are in this array
    int count = std::stoi(header.substr(1));

    if (count <= 0) {
        throw std::runtime_error("RESP parse error: array count must be positive");
    }

    // Parse each element — redis-cli always sends bulk strings
    Command cmd;
    for (int i = 0; i < count; i++) {
        cmd.args.push_back(parse_bulk_string(raw, pos));
    }

    return cmd;
}

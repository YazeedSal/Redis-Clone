#pragma once

#include <string>
#include <vector>
#include <stdexcept>

// A Command is the result of parsing a RESP message.
// args[0] is always the command name (GET, SET, DEL, etc.)
// args[1..n] are the arguments to that command.
//
// Example: "SET name Ahmed" becomes:
// Command { args: ["SET", "name", "Ahmed"] }
struct Command {
    std::vector<std::string> args;
};

// RespParser takes a raw string received from the TCP socket
// and parses it into a Command struct.
//
// For now we only parse Arrays of Bulk Strings, which is the
// format redis-cli uses to send all commands.
class RespParser {
public:
    // Takes the raw bytes received from the client.
    // Returns a fully parsed Command.
    // Throws std::runtime_error if the input is malformed.
    Command parse(const std::string& raw);

private:
    // Parses a bulk string starting at position `pos` in `raw`.
    // A bulk string looks like: $5\r\nHello\r\n
    // Updates `pos` to point to the character after this bulk string.
    std::string parse_bulk_string(const std::string& raw, size_t& pos);
};

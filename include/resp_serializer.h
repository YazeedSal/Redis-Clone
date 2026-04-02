#pragma once

#include <string>

// RespSerializer turns C++ values into RESP-formatted strings
// that can be sent back to the client over TCP.
//
class RespSerializer {
public:
    // Sends "+OK\r\n"
    // Used for commands that succeed with no return value (SET, EXPIRE etc.)
    static std::string ok();

    // Sends "-ERR <message>\r\n"
    // Used when a command fails or is unknown
    static std::string error(const std::string& message);

    // Sends "$<length>\r\n<value>\r\n"
    // Used to return a string value (GET etc.)
    static std::string bulk_string(const std::string& value);

    // Sends "$-1\r\n"
    // Used when a key doesn't exist (GET on missing key)
    // This is the RESP representation of null
    static std::string null_bulk_string();

    // Sends ":<value>\r\n"
    // Used to return an integer (DEL, EXISTS, TTL etc.)
    static std::string integer(int value);
};

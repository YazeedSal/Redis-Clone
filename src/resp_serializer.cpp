#include "resp_serializer.h"

std::string RespSerializer::ok() {
    // Simple strings start with '+'
    return "+OK\r\n";
}

std::string RespSerializer::error(const std::string& message) {
    // Errors start with '-'
    return "-ERR " + message + "\r\n";
}

std::string RespSerializer::bulk_string(const std::string& value) {
    // Bulk strings start with '$' followed by the byte length,
    // then \r\n, then the actual content, then another \r\n.
    //
    // Example: bulk_string("Hello") produces:
    // $5\r\nHello\r\n
    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
}

std::string RespSerializer::null_bulk_string() {
    // $-1\r\n is the RESP null value.
    return "$-1\r\n";
}

std::string RespSerializer::integer(int value) {
    // Integers start with ':'
    // Example: integer(42) produces :42\r\n
    return ":" + std::to_string(value) + "\r\n";
}

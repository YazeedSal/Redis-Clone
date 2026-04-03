#include "dispatcher.h"
#include <algorithm>
#include <stdexcept>

Dispatcher::Dispatcher(Store& store) : store_(store) {}

std::string Dispatcher::dispatch(const Command& cmd) {
    if (cmd.args.empty()) {
        return RespSerializer::error("empty command");
    }

    // Normalize the command name to uppercase.
    // redis-cli sends uppercase, but we handle lowercase too
    // just in case — "get" and "GET" should both work.
    std::string name = cmd.args[0];
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    if (name == "PING")   return handle_ping(cmd);
    if (name == "SET")    return handle_set(cmd);
    if (name == "GET")    return handle_get(cmd);
    if (name == "DEL")    return handle_del(cmd);
    if (name == "EXISTS") return handle_exists(cmd);
    if (name == "EXPIRE") return handle_expire(cmd);
    if (name == "TTL")    return handle_ttl(cmd);
    if (name == "KEYS")   return handle_keys(cmd);
    if (name == "INCR")   return handle_incr(cmd);
    if (name == "DECR")   return handle_decr(cmd);
    if (name == "APPEND")  return handle_append(cmd);
   
    return RespSerializer::error(
        "unknown command '" + cmd.args[0] + "'"
    );
}

std::string Dispatcher::handle_ping(const Command& cmd) {
    // PING can optionally take a message argument.
    // PING        → +PONG
    // PING hello  → $5\r\nhello\r\n (echoes the message back)
    if (cmd.args.size() > 1) {
        return RespSerializer::bulk_string(cmd.args[1]);
    }
    return "+PONG\r\n";
}

std::string Dispatcher::handle_set(const Command& cmd) {
    // SET requires exactly 2 arguments: key and value
    // SET name Ahmed
    if (cmd.args.size() < 3) {
        return RespSerializer::error(
            "wrong number of arguments for 'SET'"
        );
    }

    store_.set(cmd.args[1], cmd.args[2]);
    return RespSerializer::ok();
}

std::string Dispatcher::handle_get(const Command& cmd) {
    // GET requires exactly 1 argument: key
    // GET name
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'GET'"
        );
    }

    auto value = store_.get(cmd.args[1]);

    // std::optional — check if a value was returned
    if (!value.has_value()) {
        return RespSerializer::null_bulk_string();
    }

    return RespSerializer::bulk_string(value.value());
}

std::string Dispatcher::handle_del(const Command& cmd) {
    // DEL requires at least 1 argument: key
    // DEL name
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'DEL'"
        );
    }

    int result = store_.del(cmd.args[1]);
    return RespSerializer::integer(result);
}

std::string Dispatcher::handle_exists(const Command& cmd) {
    // EXISTS requires at least 1 argument: key
    // EXISTS name
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'EXISTS'"
        );
    }

    int result = store_.exists(cmd.args[1]);
    return RespSerializer::integer(result);
}

std::string Dispatcher::handle_expire(const Command& cmd) {
    // EXPIRE requires 2 arguments: key and seconds
    // EXPIRE name 10
    if (cmd.args.size() < 3) {
        return RespSerializer::error(
            "wrong number of arguments for 'EXPIRE'"
        );
    }

    // std::stoi converts the seconds argument from string to int.
    // We wrap it in try/catch in case the user passes a non-number.
    int seconds = 0;
    try {
        seconds = std::stoi(cmd.args[2]);
    } catch (...) {
        return RespSerializer::error("value is not an integer");
    }

    if (seconds <= 0) {
        return RespSerializer::error("invalid expire time");
    }

    int result = store_.expire(cmd.args[1], seconds);
    return RespSerializer::integer(result);
}

std::string Dispatcher::handle_ttl(const Command& cmd) {
    // TTL requires exactly 1 argument: key
    // TTL name
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'TTL'"
        );
    }

    int result = store_.ttl(cmd.args[1]);
    return RespSerializer::integer(result);
}

std::string Dispatcher::handle_keys(const Command& cmd) {
    // KEYS requires exactly 1 argument: the pattern
    // KEYS *
    // KEYS n?me
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'KEYS'"
        );
    }

    std::vector<std::string> result = store_.keys(cmd.args[1]);

    // KEYS returns a RESP array of bulk strings.
    // We need to build it manually since RespSerializer
    // doesn't have an array method yet.
    //
    // Format: *<count>\r\n followed by one bulk string per key
    std::string response = "*" + std::to_string(result.size()) + "\r\n";
    for (const auto& key : result) {
        response += RespSerializer::bulk_string(key);
    }

    return response;
}


std::string Dispatcher::handle_incr(const Command& cmd) {
    // INCR requires exactly 1 argument: the key
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'INCR'"
        );
    }

    auto result = store_.incr(cmd.args[1]);

    if (!result.ok) {
        return RespSerializer::error(result.error);
    }

    return RespSerializer::integer(result.value);
}

std::string Dispatcher::handle_decr(const Command& cmd) {
    // DECR requires exactly 1 argument: the key
    if (cmd.args.size() < 2) {
        return RespSerializer::error(
            "wrong number of arguments for 'DECR'"
        );
    }

    auto result = store_.decr(cmd.args[1]);

    if (!result.ok) {
        return RespSerializer::error(result.error);
    }

    return RespSerializer::integer(result.value);
}

std::string Dispatcher::handle_append(const Command& cmd) {
    // APPEND requires exactly 2 arguments: key and value
    if (cmd.args.size() < 3) {
        return RespSerializer::error(
            "wrong number of arguments for 'APPEND'"
        );
    }

    int new_length = store_.append(cmd.args[1], cmd.args[2]);
    return RespSerializer::integer(new_length);
}

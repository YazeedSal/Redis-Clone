#pragma once

#include <string>
#include "resp_parser.h"
#include "store.h"
#include "resp_serializer.h"

class Dispatcher {
public:
    // Dispatcher needs a reference to the store.
    // The store is owned by the Server and shared across all threads.
    explicit Dispatcher(Store& store);

    // Takes a parsed Command, executes it against the store,
    // and returns a RESP-formatted response ready to send back.
    std::string dispatch(const Command& cmd);

private:
    Store& store_;

    // One private method per command group
    std::string handle_ping(const Command& cmd);
    std::string handle_set(const Command& cmd);
    std::string handle_get(const Command& cmd);
    std::string handle_del(const Command& cmd);
    std::string handle_exists(const Command& cmd);
    std::string handle_expire(const Command& cmd);
    std::string handle_ttl(const Command& cmd);
};

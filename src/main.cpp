#include "server.h"
#include <iostream>

int main() {
    try {
        // 6379 is the default Redis port
        Server server(6379);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

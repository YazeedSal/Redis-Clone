
#include "server.h"

#include <iostream>
#include <string>
#include <thread>
#include <stdexcept>

// POSIX socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

Server::Server(int port) : port_(port), server_fd_(-1) {}

void Server::setup_socket() {
    // 1. Create the socket file descriptor
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // 2. Allow reuse of the port immediately after restart
    //    Without this, you get "Address already in use" during dev
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Bind to the port on all interfaces
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0
    addr.sin_port        = htons(port_);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind to port " + std::to_string(port_));
    }

    // 4. Start listening — backlog of 10 pending connections
    if (listen(server_fd_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server listening on port " << port_ << "\n";
}

void Server::accept_loop() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // Blocks here until a client connects
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        std::cout << "New client connected (fd=" << client_fd << ")\n";

        // Spawn a thread per client — each gets its own handle_client call
        std::thread(&Server::handle_client, this, client_fd).detach();
    }
}

void Server::handle_client(int client_fd) {
    char buffer[4096];

    while (true) {
        // Read data from the client
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            // 0 = client disconnected, <0 = error
            std::cout << "Client disconnected (fd=" << client_fd << ")\n";
            close(client_fd);
            return;
        }

        buffer[bytes_read] = '\0';
        std::string message(buffer, bytes_read);

        // Echo it straight back — this is temporary
        send(client_fd, message.c_str(), message.size(), 0);
    }
}

void Server::start() {
    setup_socket();
    accept_loop();
}

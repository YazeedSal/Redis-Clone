

#pragma once

#include <string>

class Server {
	public:
		Server(int port);
		void start();

	private:
		int port_;
	        int server_fd_;

	        void setup_socket();
	        void accept_loop();
       	        void handle_client(int client_fd);
};




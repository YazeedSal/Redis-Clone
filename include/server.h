

#pragma once

#include <string>
#include "resp_parser.h"
#include "resp_serializer.h"

class Server {
	public:
		Server(int port);
		void start();

	private:
		int port_;
	        int server_fd_;

		// The parser and serializer are shared across all client threads.
    		// They are stateless so no mutex is needed for them.
    		RespParser parser_;
    		RespSerializer serializer_;

	        void setup_socket();
	        void accept_loop();
       	        void handle_client(int client_fd);
};




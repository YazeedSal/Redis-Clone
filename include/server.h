

#pragma once

#include <string>
#include "resp_parser.h"
#include "resp_serializer.h"
#include "store.h"
#include "dispatcher.h"

class Server {
	public:
		Server(int port);
		void start();

	private:
		int port_;
	        int server_fd_;

		Store store_;
    		RespParser parser_;
    		Dispatcher dispatcher_;

	        void setup_socket();
	        void accept_loop();
       	        void handle_client(int client_fd);
};




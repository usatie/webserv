#include "HTTP_Server.h"
#include <stdio.h> // printf()
#include <sys/socket.h> // socket()
#include <netinet/in.h> // struct sockaddr_in
#include "error.h"
#define BACKLOG 5

void init_server(HTTP_Server * http_server, int port) {
	http_server->port = port;

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		errExit("socket");
	}

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	
	// resolve: bindAddress already in use
	int optval = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		errExit("setsockopt");
	}

	if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
		errExit("bind");
	}
	// BACKLOG: the maximum number of pending connections that can be queued up before connections are refused.
	if (listen(server_socket, BACKLOG) == -1) {
		errExit("listen");
	}

	http_server->socket = server_socket;
	printf("HTTP Server Initialized\nPort: %d\n", port);
}

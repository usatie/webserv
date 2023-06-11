#include <stdio.h> // printf()
#include <sys/socket.h> // socket()
#include <netinet/in.h> // struct sockaddr_in
#include <string.h> // strtok()
#include <unistd.h> // read()
#include <stdlib.h> // exit()
#include "HTTP_Server.h"
#include "Routes.h"
#include "Response.h"
#include "error.h"

int main() {
	// initiate HTTP_Server
	HTTP_Server http_server;
	init_server(&http_server, 6969);

	int client_socket;

	// registering Routes
	struct Route *route = initRoute("/", "index.html");
	addRoute(route, "/about", "about.html");

	printf("\n========================================\n");
	printf("======== Registered Routes ==============\n");
	// display all available routes
	inorder(route);

	while (1) {
		char client_msg[4096] = "";

		client_socket = accept(http_server.socket, NULL, NULL);
		read(client_socket, client_msg, sizeof(client_msg));
		printf("%s\n", client_msg);
		char *method = "";
		char *urlRoute = "";

		char *client_http_header = strtok(client_msg, "\n");
		printf("\n\n%s\n\n", client_http_header);
		char *header_token = strtok(client_http_header, " ");
		int header_parse_counter = 0;

		while (header_token != NULL) {
			switch (header_parse_counter) {
				case 0:
					method = header_token;
				case 1:
					urlRoute = header_token;
			}
			header_token = strtok(NULL, " ");
			header_parse_counter++;
		}
		printf("Method: %s\n", method);
		printf("URL Route: %s\n", urlRoute);

		char template[100] = "";
		if (strstr(urlRoute, "/static/") != NULL) {
			strcat(template, "static/index.css");
		} else {
			struct Route *destination = search(route, urlRoute);
			strcat(template, "templates/");
			if (destination == NULL) {
				strcat(template, "404.html");
			} else {
				strcat(template, destination->value);
			}
		}

		printf("Template: %s\n", template);
		char *response_data = render_static_file(template);
		char http_header[4096] = "";
		if (response_data == NULL) {
			strcat(http_header, "HTTP/1.1 404 Not Found\r\n\r\n");
			response_data = render_static_file("templates/404.html");
		} else {
			strcat(http_header, "HTTP/1.1 200 OK\r\n\r\n");
		}

		strcat(http_header, response_data);
		strcat(http_header, "\r\n\r\n");

		send(client_socket, http_header, sizeof(http_header), 0);
		close(client_socket);
		free(response_data);
	}
	return 0;
}

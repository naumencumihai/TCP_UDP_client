#include "helpers.h"

int main(int argc, char *argv[]) {
	// Check for correct usage
	DIE(argc != 4, "Incorrect usage\nUsage example:\
	./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>\n");

	// Deactivate output buffer
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	char send_buffer[INBUFLEN], receive_buffer[sizeof(struct UDP2TCP)];
	struct UDP2TCP *UDP_message;
	struct subscribe_command sub_command;

	// Initialize server
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));

	fd_set fds, tmp_fds;
	FD_ZERO(&fds);

	// Socket for SERVER connection
	int SERV_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(SERV_socket < 0, "SERVER socket initialization failed");

	// Fill server information
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argv[3]));
	int ret = inet_aton(argv[2], &server_address.sin_addr);
	DIE(ret == 0, "Wrong server IP\n");

	// Establish connection to server
	ret = connect(SERV_socket, (struct sockaddr *)&server_address, 
						sizeof(server_address));
	DIE(ret < 0, "Connection to server could not be established\n");

	// Send ID to server so that the server can remember the client
	ret = send(SERV_socket, argv[1], 1 + strlen(argv[1]), 0);
	DIE(ret < 0, "Sending ID to server failed\n						\
			ID length cannot be more than 10 ASCII characters!\n");

	// Disable Nagle's algorithm
	int optval = 1;
	ret = setsockopt(SERV_socket, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
	DIE(ret < 0, "Set socket option failed");

	// Set file descriptors
	FD_SET(SERV_socket, &fds);
	FD_SET(0, &fds);

	bool server_UP = true, client_UP = true;
	while (server_UP && client_UP) {
		tmp_fds = fds;
		ret = select(SERV_socket + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Select socket failed\n");

		// Receive command (from stdin)
		if (FD_ISSET(0, &tmp_fds)) {
			// Clear read buffer
			memset(send_buffer, 0, INBUFLEN);
			// Read from stdin
			fgets(send_buffer, INBUFLEN, stdin);

			// Received "exit" command from stdin
			if (!strncmp(send_buffer, "exit\n", 5)) {
				// Send command to server, so it can disconnect the client
				strcat(send_buffer, argv[1]);
				ret = send(SERV_socket, (char *)&send_buffer, sizeof(send_buffer), 0);
				DIE(ret < 0, "Sending command to server failed\n");
				client_UP = false;
				break;
			// Received "subscribe" command from stdin
			} else if (!strncmp(send_buffer, "subscribe", 9)) {
				// parse through command
				char *token = strtok(send_buffer, " ");
				if (token == NULL) {
					printf("Usage: subscribe <TOPIC> <SF>\n");
					continue;
				}
				char cmd[CMDMAXLEN];
				strcpy(cmd, token);
				token = strtok(NULL, " ");
				if (token == NULL) {
					printf("Usage: subscribe <TOPIC> <SF>\n");
					continue;
				}
				strcpy(sub_command.topic, token);
				if (strlen(sub_command.topic) == TOPICMAXLEN) {
					sub_command.topic[TOPICMAXLEN] = '\0';
				}
				strcpy(sub_command.command, cmd);
				token = strtok(NULL, "\n");
				if (token == NULL) {
					printf("Usage: subscribe <TOPIC> <SF>\n");
					continue;
				}
				if (strcmp(token, "0") && strcmp(token, "1")) {
					printf("SF value must be 0 or 1\n");
					continue;
				}
				sub_command.sf = atoi(token);
				strcpy(sub_command.id, argv[1]);
				
				ret = send(SERV_socket, (char*)&sub_command, sizeof(sub_command), 0);
				DIE(ret < 0, "Sending command to server failed\n");

				printf("Subscribed to topic.\n");
			// Received "unsubscribe" command from stdin
			} else if (!strncmp(send_buffer, "unsubscribe", 11)) {
				// parse through command
				char *token = strtok(send_buffer, " ");
				if (token == NULL) {
					printf("Usage: unsubscribe <TOPIC>\n");
					continue;
				}
				char cmd[CMDMAXLEN];
				strcpy(cmd, token);
				token = strtok(NULL, "\n");
				if (token == NULL) {
					printf("Usage: unsubscribe <TOPIC>\n");
					continue;
				}
				strcpy(sub_command.topic, token);
				if (strlen(sub_command.topic) == TOPICMAXLEN) {
					sub_command.topic[TOPICMAXLEN] = '\0';
				}
				strcpy(sub_command.command, cmd);
				strcpy(sub_command.id, argv[1]);

				ret = send(SERV_socket, (char*)&sub_command, sizeof(sub_command), 0);
				DIE(ret < 0, "Sending command to server failed\n");

				printf("Unsubscribed from topic.\n");
			}
		}
		// Receive message from server (UDP message)
		if (FD_ISSET(SERV_socket, &tmp_fds)) {
			// Clear receive buffer
			memset(receive_buffer, 0, sizeof(struct UDP2TCP));

            ret = recv(SERV_socket, receive_buffer, sizeof(struct UDP2TCP), 0);
            DIE(ret < 0, "Receiving from server failed\n");

			// Server is DOWN
			if (!ret) {
				server_UP = false;
				break;
			}

			// Unload received buffer to a predefined struct
			UDP_message = (struct UDP2TCP *) receive_buffer;

			// format: <IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <DATA_TYPE> - <CONTENT>
			printf("%s:%d - %s - ", 
								UDP_message->ip, 
								UDP_message->portno, 
								UDP_message->received_message.topic);

			uint8_t data_type = (uint8_t)UDP_message->received_message.data_type;
			char type_identifier[TYPEMAXLEN];
			// Determines the type identifier based on received data type
			switch (data_type) {
				case 0:
					strcpy(type_identifier, "INT");
					break;
				case 1:
					strcpy(type_identifier, "SHORT_REAL");
					break;
				case 2:
					strcpy(type_identifier, "FLOAT");
					break;
				case 3:
					strcpy(type_identifier, "STRING");
					break;
				default:
					break;
			}
			printf("%s - %s\n", type_identifier, UDP_message->received_message.content);

		}
	}
	close(SERV_socket);
	return 0;
}

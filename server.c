#include "helpers.h"

int main(int argc, char *argv[]) {
	// Check for correct usage
	DIE(argc != 2, "Incorrect usage\nUsage example: ./server <PORT>\n");

	// Deactivate output buffer
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// List of clients logged on server (they can be online or offline)
	struct client clients[MAXCLIENTS];
	int clients_no = 0;

	// List of queued messages (so clients cand receive if they use SF)
	struct queued_message queued_messages[MAXMESSAGES];
	int queued_messages_no = 0;

	bool server_UP = true;
	int NEW_socket;
	struct sockaddr_in cli_addr;
	struct UDP2TCP to_TCP;
	struct message *msg;
	socklen_t clilen, udplen;
	map_int_t subs;

	// Socket for UDP client connection
	int UDP_socket = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(UDP_socket < 0, "UDP socket initialization failed\n");

	// Socket for TCP client connection
	int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(TCP_socket < 0, "TCP socket initialization failed\n");

	// Initialize server
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	// Fill server information
	int port_no = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_no);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind UDP socket
	int ret = bind(UDP_socket,(struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in));
	DIE(ret < 0, "Binding UDP socket with the server address failed\n");

	// Bind TCP socker
	ret = bind(TCP_socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in));
	DIE(ret < 0, "Binding TCP socket with the server address failed\n");

	// Listen on TCP socket
	ret = listen(TCP_socket, 1);
	DIE(ret < 0, "Listen failed\n");

	// Define file descriptors sets (for reading and a temporary one)
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax, prev_max;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// Add TCP and UDP file descriptors to set
	FD_SET(TCP_socket, &read_fds);
	FD_SET(UDP_socket, &read_fds);
	FD_SET(0, &read_fds);

	fdmax = max(TCP_socket, UDP_socket);

	// Free port on server closing
	int optval = 1;
	ret = setsockopt(TCP_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	DIE(ret < 0, "Set socket option failed\n");

	// Disable Nagle's algorithm
	ret = setsockopt(TCP_socket, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
	DIE(ret < 0, "Set socket option failed\n");

	char buffer[INBUFLEN];
	memset(buffer, 0, sizeof(buffer));

	while (server_UP) {
		tmp_fds = read_fds;
		memset(buffer, 0, sizeof(buffer));
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Select socket failed\n");

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// Receive command from stdin (only exit command available)
				if (i == 0) {
					fgets(buffer, INBUFLEN, stdin);
					if (!strncmp(buffer , "exit\n", 5)) {
						server_UP = false;
						break;
					}
				// Receive message from UDP client
				} else if (i == UDP_socket) {
					udplen = sizeof(struct sockaddr);
					ret = recvfrom(UDP_socket, (struct message*)buffer, sizeof(buffer), 0, 
									(struct sockaddr*)&serv_addr, &udplen);
					DIE(ret < 0, "Receiving from UDP client failed\n");

					// Add ip address and port number to forward to TCP client
					strcpy(to_TCP.ip, inet_ntoa(serv_addr.sin_addr));
					to_TCP.portno = ntohs(serv_addr.sin_port);

					msg = (struct message*) buffer;
					strcpy(to_TCP.received_message.topic, msg->topic);
					to_TCP.received_message.data_type = msg->data_type;
					strcpy(to_TCP.received_message.content, msg->content);

					// Decode message content received from UDP client
					uint8_t data_type = (uint8_t)to_TCP.received_message.data_type;
					uint8_t sign;
					uint8_t power;
					uint32_t number32;
					uint16_t number16;

					// Decode message content
					switch (data_type) {
						// INT
						case 0:
							sign = msg->content[0];
							memcpy(&number32, &msg->content[1], sizeof(uint32_t));
							int result = pow(-1, sign) * ntohl(number32);
							sprintf(to_TCP.received_message.content, "%d", result);
							break;
						// SHORT_REAL
						case 1:
							memcpy(&number16, &msg->content[0], sizeof(uint16_t));
							float result_float = ntohs(number16) / (float)100;
							sprintf(to_TCP.received_message.content, "%.2f", result_float);
							break;
						// FLOAT
						case 2:
							sign = msg->content[0];
							memcpy(&number32, &msg->content[sizeof(uint8_t)], sizeof(uint32_t));
							int size = sizeof(uint8_t) + sizeof(uint32_t);
							memcpy(&power, &msg->content[size], sizeof(uint8_t));
							float result_float2 = pow(-1, sign) * ntohl(number32) * pow(10, (-1)*power);
							sprintf(to_TCP.received_message.content, "%.4f", result_float2);
							break;
						// STRING									
						default:
							break;
					}
					char topic[TOPICMAXLEN];
					strcpy(topic, to_TCP.received_message.topic);
					const char *key;
					// Send message to online TCP clients
					for (int j = 0; j < clients_no; j++) {
						if (clients[j].online == true) {
							map_iter_t iter = map_iter(&clients[j].subscriptions);
							while ((key = map_next(&clients[j].subscriptions, &iter))) {
								// If it has topic in subsciption list
								if (!strcmp(key, topic)) {
									// Send content
									ret = send(j + CLIENTPORT, (char*)&to_TCP, sizeof(to_TCP), 0);
								}
							}
						// For offline clients, add to their subscriptions with sf enabled
						} else {
							map_iter_t iter = map_iter(&clients[j].subscriptions);
							while ((key = map_next(&clients[j].subscriptions, &iter))) {
								// If it has topic in subsciption list and has SF activated for it
								if (!strcmp(key, topic) && *map_get(&clients[j].subscriptions, key) == 1) {
									struct queued_message q_msg;
									strcpy(q_msg.ip, to_TCP.ip);
									q_msg.portno = to_TCP.portno;
									strcpy(q_msg.content, to_TCP.received_message.content);
									q_msg.data_type = to_TCP.received_message.data_type;
									strcpy(q_msg.topic, to_TCP.received_message.topic);
									strcpy(q_msg.id, clients[j].id);

									queued_messages[queued_messages_no] = q_msg;
									queued_messages_no++;
								}
							}
						}
					}
				// Receive connection request form TCP client
				} else if (i == TCP_socket) {
					clilen = sizeof(cli_addr);
					NEW_socket = accept(TCP_socket, (struct sockaddr *)&cli_addr,
							&clilen);
					DIE(NEW_socket < 0, "Accepting connection from client failed\n");

					// Disable Nagle's algorithm
					setsockopt(NEW_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(int));

					// Add new socket to file descriptors set
					FD_SET(NEW_socket, &read_fds);
					if (NEW_socket > fdmax) {
						prev_max = fdmax;
						fdmax = NEW_socket;
					}
					// Receive subscriber's ID
					memset(buffer, 0, sizeof(buffer));
					ret = recv(NEW_socket, buffer, sizeof(buffer), 0);
					DIE(ret < 0, "Receiving from subscriber failed\n");

					bool client_logged = false;
					int current_client;
					// Verify if ID exists (all clients offline & online)
					for (int j = 0; j < clients_no; j++) {
						if (!strcmp(clients[j].id, buffer)) {
							client_logged = true;
							current_client = j;
							break;
						}
					}
					// If not, add new client
					if (client_logged == false) {
						printf("New client %s connected from %s:%d\n", 
						buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						
						// Add new client to clients list
						struct client new_client;
						strcpy(new_client.id, buffer);
						new_client.online = true;
  						map_init(&subs);
						new_client.subscriptions = subs;
						clients[clients_no] = new_client;
						current_client = clients_no;
						clients_no++;
					// If client already logged
					} else {
						// If logged client is online
						if (clients[current_client].online == true) {
							printf("Client %s already connected\n", buffer);
							fdmax = prev_max;
							close(NEW_socket);
							continue;
						// If logged client is offline, it allows it to connect
						} else {
							printf("New client %s connected from %s:%d\n", 
									buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
							struct client tmp_client;
							tmp_client = clients[current_client];
							clients[current_client].online = true;
							int sf = 0;
							const char *key;
							map_iter_t iter = map_iter(&tmp_client.subscriptions);
							while ((key = map_next(&tmp_client.subscriptions, &iter))) {
								sf = *map_get(&tmp_client.subscriptions, key);
							}
							// Client uses SF on any subscription
							if (sf) {
								for (int j = 0; j < queued_messages_no; j++) {
									if (!strcmp(queued_messages[j].id, clients[current_client].id)) {
										struct UDP2TCP tmp_msg;
										strcpy(tmp_msg.ip, queued_messages[j].ip);
										tmp_msg.portno = queued_messages[j].portno;
										strcpy(tmp_msg.received_message.topic, queued_messages[j].topic);
										tmp_msg.received_message.data_type = queued_messages[j].data_type;
										strcpy(tmp_msg.received_message.content, queued_messages[j].content);

										ret = send(current_client + CLIENTPORT, (char*)&tmp_msg, sizeof(tmp_msg), 0);
										DIE(ret < 0, "Sending to TCP client failed\n");

										// Remove message from queued messages list
										remove_message(queued_messages, j, queued_messages_no);
										queued_messages_no--;
										j--;
									}
								}
							}
						}
					}
				// Receive command from TCP client
				} else {
					// Clear buffer
					memset(buffer, 0, sizeof(buffer));
					ret = recv(i, buffer, sizeof(buffer), 0);
					DIE(ret < 0, "Receiving command from TCP client failed\n");
					
					// Client sends exit command, server disconnects it
					if (!strncmp(buffer, "exit\n", 5)) {
						// exit command from TCP client has format:
						// exit\n<CLIENT_ID>
						memmove(buffer, buffer + 5, strlen(buffer));
						for (int j = 0; j < clients_no; j++) {	
							if (!strcmp(clients[j].id, buffer)) {
								clients[j].online = false;
								printf("Client %s disconnected.\n", clients[j].id);
								break;
							}
						}
						close(i);
						FD_CLR(i, &read_fds);
					// Client sends subscribe/unsubscribe command
					} else {
						struct subscribe_command *received = (struct subscribe_command*) buffer;
						
						// Received subscribe command
						if (!strcmp(received->command, "subscribe")) {
							for (int j = 0; j < clients_no; j++) {
								if (!strcmp(clients[j].id, received->id)) {
									if (strlen(received->topic) - strlen("subscribe") == TOPICMAXLEN) {
										received->topic[TOPICMAXLEN] = '\0';
									}
									map_set(&clients[j].subscriptions, 
																	received->topic, 
																	received->sf);
								}
							}
						// Received unsubscribe command
						} else if (!strcmp(received->command, "unsubscribe")) {
							for (int j = 0; j < clients_no; j++) {	
								if (!strcmp(clients[j].id, received->id)) {
									if (strlen(received->topic) - strlen("unsubscribe") == TOPICMAXLEN) {
										received->topic[TOPICMAXLEN] = '\0';
									}
									map_remove(&clients[j].subscriptions, 
																	received->topic);

								}
							}
						}
					}
				}
			}
		}
	}
	if (&subs) {
		map_deinit(&subs);
	}

	// Close all connected clients
	for (int i = 3; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fds)) {
            close(i);
        }
    }
	return 0;
}

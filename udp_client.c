#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAGIC_NUMBER_1 'J'
#define MAGIC_NUMBER_2 'T'

#define STATE_OFFLINE 0
#define STATE_LOGIN_SENT 1
#define STATE_ONLINE 2
#define STATE_LOGOUT_SENT 3
#define STATE_POST_SENT 4
#define STATE_SUBSCRIBE_SENT 5
#define STATE_UNSUBSCRIBE_SENT 6
#define STATE_RETRIEVE_SENT 7

#define EVENT_LOGIN 1
#define EVENT_SUCCESSFUL_LOGIN 2
#define EVENT_UNSUCCESSFUL_LOGIN 3
#define EVENT_LOGOUT 4
#define EVENT_SUCCESSFUL_LOGOUT 5
#define EVENT_POST 6
#define EVENT_POST_ACK 7
#define EVENT_SUBSCRIBE 8
#define EVENT_SUBSCRIBE_ACK 9
#define EVENT_UNSUBSCRIBE 10
#define EVENT_UNSUBSCRIBE_ACK 11
#define EVENT_RETRIEVE 12
#define EVENT_RETRIEVE_ACK 13
#define EVENT_RETRIEVE_END_ACK 14
#define EVENT_FORWARD 15
#define EVENT_UNKNOWN 99

int main(int argc, char *argv[]) {
	int connection_status;
	int ret;
	int sockfd_tx = 0;
	int sockfd_rx = 0;
	char send_buffer[1024];
	char recv_buffer[1024];
	struct sockaddr_in serv_addr;
	struct sockaddr_in my_addr;
	int maxfd;
	fd_set read_set;
	FD_ZERO(&read_set);

	sockfd_tx = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_tx < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(32000);

	sockfd_rx = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_rx < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&my_addr, 0, sizeof(serv_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	my_addr.sin_port = htons(31000);

	bind(sockfd_rx, (struct sockaddr *) &my_addr, sizeof(my_addr));
	connection_status = connect(sockfd_tx, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (connection_status < 0) {
		printf("connect() error: %s.\n", strerror(errno));
		return -1;
	}

	maxfd = sockfd_rx + 1;

	int state = STATE_OFFLINE;
	int event;
	int token;

	while (1) {

		FD_SET(fileno(stdin), &read_set);
		FD_SET(sockfd_rx, &read_set);

		select(maxfd, &read_set, NULL, NULL, NULL);

		if (FD_ISSET(fileno(stdin), &read_set)) {
			fgets(send_buffer, sizeof(send_buffer), stdin);
			if (strncmp(send_buffer, "login#", strlen("login#")) == 0) {
				event = EVENT_LOGIN;
			} else if (strncmp(send_buffer, "post#", strlen("post#")) == 0) {
				event = EVENT_POST;
			} else if (strncmp(send_buffer, "retrieve#", strlen("retrieve#")) == 0) {
				event = EVENT_RETRIEVE;
			} else if (strncmp(send_buffer, "logout#", strlen("logout#")) == 0) {
				event = EVENT_LOGOUT;
			} else event = EVENT_UNKNOWN;

			if (event == EVENT_LOGIN) {
				
			}
		}

		if (FD_ISSET(sockfd_rx, &read_set)) {

		}

		int m = 0;
		if (strncmp(user_input, "post#", 5) == 0) {
			m = strlen(user_input) - 5;
			memcpy(send_buffer+4, user_input+5, m);

			send_buffer[0] = 'J';
			send_buffer[1] = 'T';
			send_buffer[2] = 1;
			send_buffer[3] = m;
		} else if (strncmp(user_input, "retrieve#", 9) == 0) {
			m = strlen(user_input) - 9;
			memcpy(send_buffer+4, user_input+9, m);

			send_buffer[0] = 'J';
			send_buffer[1] = 'T';
			send_buffer[2] = 3;
			send_buffer[3] = m;
		} else {
			printf("Error: Unrecognized command format.\n");
			continue;
		}
		ret = send(sockfd, send_buffer, m+4, 0);
	
		if (ret <= 0) {
			printf("send() error: %s.\n", strerror(errno));
			return -1;
		}	

		char server_response[1024];
		recv(sockfd, &server_response, sizeof(server_response), 0);

		if (server_response[0] != 'J' || server_response[1] != 'T') {
			continue;
		} else {
			if (server_response[2] == 2) {
				printf("%s\n", server_response+4);
			} else if (server_response[2] == 4) {
				printf("%s", server_response+4);
			} else {
				continue;
			}
		}
	}

    close(sockfd);

    return 0;
}

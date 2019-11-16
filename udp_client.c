#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct header {
	char magic1;
	char magic2;
	char opcode;
	char payload_length;

	uint32_t token;
	uint32_t msg_id;
};

const int h_size = sizeof(struct header);

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

#define OPCODE_SESSION_RESET 0x00
#define OPCODE_MUST_LOGIN_FIRST 0xF0
#define OPCODE_LOGIN 0x10
#define OPCODE_LOGIN_SUCCESSFUL_ACK 0x80
#define OPCODE_LOGIN_FAILED_ACK 0x81
#define OPCODE_SUBSCRIBE 0x20
#define OPCODE_SUBSCRIBE_SUCCESSFUL_ACK 0x90
#define OPCODE_SUBSCRIBE_FAILED_ACK 0x91
#define OPCODE_UNSUBSCRIBE 0x21
#define OPCODE_UNSUBSCRIBE_SUCCESSFUL_ACK 0xA0
#define OPCODE_UNSUBSCRIBE_FAILED_ACK 0xA1
#define OPCODE_POST 0x30
#define OPCODE_POST_ACK 0xB0
#define OPCODE_FORWARD 0xB1
#define OPCODE_FORWARD_ACK 0x31
#define OPCODE_RETRIEVE 0x40
#define OPCODE_RETRIEVE_ACK 0xC0
#define OPCODE_RETRIEVE_END_ACK 0xC1
#define OPCODE_LOGOUT 0x1F
#define OPCODE_LOGOUT_ACK 0x8F

int main(int argc, char *argv[]) {
	int connection_status;
	int ret;
	int sockfd = 0;
	char user_input[1024];
	char send_buffer[1024];
	char recv_buffer[1024];
	struct sockaddr_in serv_addr;
	struct sockaddr_in my_addr;
	int maxfd;
	fd_set read_set;
	FD_ZERO(&read_set);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(32000);

	memset(&my_addr, 0, sizeof(serv_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	my_addr.sin_port = htons(31000);

	bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
	// connection_status = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	// if (connection_status < 0) {
	// 	printf("connect() error: %s.\n", strerror(errno));
	// 	return -1;
	// }

	maxfd = sockfd + 1;

	int state = STATE_OFFLINE;
	int event;
	uint32_t token;

	struct header *ph_send = (struct header *) send_buffer;
	struct header *ph_recv = (struct header *) recv_buffer;
	while (1) {

		FD_SET(fileno(stdin), &read_set);
		FD_SET(sockfd, &read_set);

		select(maxfd, &read_set, NULL, NULL, NULL);

		if (FD_ISSET(fileno(stdin), &read_set)) {
			fgets(user_input, sizeof(user_input), stdin);
			event = parse_event_from_input(user_input);

			if (event == EVENT_LOGIN) {
				printf("INSIDE EVENT_LOGIN\n");
				if (state == STATE_OFFLINE) {
					printf("INSIDE EVENT_LOGIN + STATE_OFFLINE\n");
					char *username_password = user_input + 6;
					int m = strlen(username_password);

					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send-> opcode = OPCODE_LOGIN;
					ph_send->payload_length = m;
					ph_send->token = 0;
					ph_send->msg_id = 0;

					memset(send_buffer, 0, sizeof(send_buffer));
					memcpy(send_buffer + h_size, username_password, m);

					sendto(sockfd, send_buffer, h_size + m, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
					state = STATE_LOGIN_SENT;
				} else {
					printf("Already logged in.\n");
					continue;
				}
			}
		}

		if (FD_ISSET(sockfd, &read_set)) {

		}
	}

    close(sockfd);

    return 0;
}

int parse_event_from_input(char user_input[]) {
	if (strncmp(user_input, "login#", strlen("login#")) == 0) {
		return EVENT_LOGIN;
	} else if (strncmp(user_input, "post#", strlen("post#")) == 0) {
		return EVENT_POST;
	} else if (strncmp(user_input, "retrieve#", strlen("retrieve#")) == 0) {
		return EVENT_RETRIEVE;
	} else if (strncmp(user_input, "logout#", strlen("logout#")) == 0) {
		return EVENT_LOGOUT;
	} else return EVENT_UNKNOWN;
}

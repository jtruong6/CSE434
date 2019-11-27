#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

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
#define STATE_MESSAGE_FORWARD 8

#define EVENT_MUST_LOGIN_FIRST 1
#define EVENT_LOGIN 2
#define EVENT_SUCCESSFUL_LOGIN 3
#define EVENT_FAILED_LOGIN 4
#define EVENT_LOGOUT 5
#define EVENT_SUCCESSFUL_LOGOUT 6
#define EVENT_POST 7
#define EVENT_POST_ACK 8
#define EVENT_SUBSCRIBE 9
#define EVENT_SUCCESSFUL_SUBSCRIBE_ACK 10
#define EVENT_FAILED_SUBSCRIBE_ACK 11
#define EVENT_UNSUBSCRIBE 12
#define EVENT_SUCCESSFUL_UNSUBSCRIBE_ACK 13
#define EVENT_FAILED_UNSUBSCRIBE_ACK 14
#define EVENT_RETRIEVE 15
#define EVENT_RETRIEVE_ACK 16
#define EVENT_RETRIEVE_END_ACK 17
#define EVENT_FORWARD 18
#define EVENT_FORWARD_ACK 19
#define EVENT_UNKNOWN 99

#define OPCODE_SESSION_RESET 0
#define OPCODE_MUST_LOGIN_FIRST 240
#define OPCODE_LOGIN 10
#define OPCODE_LOGIN_SUCCESSFUL_ACK 80
#define OPCODE_LOGIN_FAILED_ACK 81
#define OPCODE_SUBSCRIBE 20
#define OPCODE_SUBSCRIBE_SUCCESSFUL_ACK 90
#define OPCODE_SUBSCRIBE_FAILED_ACK 91
#define OPCODE_UNSUBSCRIBE 21
#define OPCODE_UNSUBSCRIBE_SUCCESSFUL_ACK 160
#define OPCODE_UNSUBSCRIBE_FAILED_ACK 161
#define OPCODE_POST 30
#define OPCODE_POST_ACK 176
#define OPCODE_FORWARD 177
#define OPCODE_FORWARD_ACK 31
#define OPCODE_RETRIEVE 40
#define OPCODE_RETRIEVE_ACK 192
#define OPCODE_RETRIEVE_END_ACK 193
#define OPCODE_LOGOUT 41
#define OPCODE_LOGOUT_ACK 143

int parse_event_from_input(char user_input[]);
int parse_event_from_recv_message(struct header* ph_recv);
void send_reset(int sockfd, char send_buffer[], struct sockaddr_in serv_addr);

int main(int argc, char *argv[]) {
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

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	my_addr.sin_port = htons(31000);

	ret = ::bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
	if (ret < 0) {
		printf("Binding error!!!");
	}
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

					memset(send_buffer, 0, sizeof(send_buffer));
					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send->opcode = OPCODE_LOGIN;
					ph_send->payload_length = m;
					ph_send->token = 0;
					ph_send->msg_id = 0;

					memcpy(send_buffer + h_size, username_password, m);

					sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
					state = STATE_LOGIN_SENT;
				} else {
					printf("Already logged in.\n");
					continue;
				}
			} else if (event == EVENT_POST) {
				if (state == STATE_ONLINE) {
					printf("INSIDE EVENT_POST + STATE_ONLINE\n");
					char *text = user_input + 5;
					int m = strlen(text);

					memset(send_buffer, 0, sizeof(send_buffer));
					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send->opcode = OPCODE_POST;
					ph_send->payload_length = m;
					ph_send->token = token;
					ph_send->msg_id = 0;

					memcpy(send_buffer + h_size, text, m);

					sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
					state = STATE_POST_SENT;
				} else {
					printf("error#must_login_first\n");
					continue;
				} 
			} else if (event == EVENT_SUBSCRIBE) {
				if (state == STATE_ONLINE) {
					printf("INSIDE EVENT_SUBSCRIBE + STATE_ONLINE\n");
					char *client = user_input + 10;
					int m = strlen(client);

					memset(send_buffer, 0, sizeof(send_buffer));
					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send->opcode = OPCODE_SUBSCRIBE;
					ph_send->payload_length = m;
					ph_send->token = token;
					ph_send->msg_id = 0;

					memcpy(send_buffer + h_size, client, m);

					sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
					state = STATE_SUBSCRIBE_SENT;
				} else {
					printf("error#must_login_first\n");
					continue;
				} 
			} else if (event == EVENT_UNSUBSCRIBE) {
				if (state == STATE_ONLINE) {
					printf("INSIDE EVENT_UNSUBSCRIBE + STATE_ONLINE\n");
					char *client = user_input + 12;
					int m = strlen(client);

                    memset(send_buffer, 0, sizeof(send_buffer));
					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send->opcode = OPCODE_UNSUBSCRIBE;
					ph_send->payload_length = m;
					ph_send->token = token;
					ph_send->msg_id = 0;

					memcpy(send_buffer + h_size, client, m);

					sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
					state = STATE_UNSUBSCRIBE_SENT;
				} else {
					printf("error#must_login_first\n");
					continue;
				} 
			} else if (event == EVENT_RETRIEVE) {
				if (state == STATE_ONLINE) {
					printf("INSIDE EVENT_RETRIEVE + STATE_ONLINE\n");
					char *n = user_input + 9;
					int m = strlen(n);
					int num = 0;

					if (isdigit(*n)) num = atoi(n);
                    else {
                        printf("Please enter a valid integer\n");
                        continue;
                    }

					memset(send_buffer, 0, sizeof(send_buffer));
					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send->opcode = OPCODE_RETRIEVE;
					ph_send->payload_length = m;
					ph_send->token = token;
					ph_send->msg_id = 0;

					memcpy(send_buffer + h_size, &num, m);

					sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
					state = STATE_RETRIEVE_SENT;
				} else {
					printf("error#must_login_first\n");
					continue;
				} 
			} else if (event == EVENT_FORWARD_ACK) {
				if (state == STATE_ONLINE) {
					printf("INSIDE EVENT_FORWWARD_ACK + STATE_ONLINE\n");

					memset(send_buffer, 0, sizeof(send_buffer));
					ph_send->magic1 = MAGIC_NUMBER_1;
					ph_send->magic2 = MAGIC_NUMBER_2;
					ph_send->opcode = OPCODE_FORWARD_ACK;
					ph_send->payload_length = 0;
					ph_send->token = token;
					ph_send->msg_id = 0;

					sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
				} else {
					printf("error#must_login_first\n");
					continue;
				}
			} else if (event == EVENT_UNKNOWN) {
				send_reset(sockfd, send_buffer, serv_addr);
				state = STATE_OFFLINE;
			}  
		}

		if (FD_ISSET(sockfd, &read_set)) {
			ret = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);
			event = parse_event_from_recv_message(ph_recv);

			if (event == EVENT_MUST_LOGIN_FIRST) {
				if (state == STATE_OFFLINE) {
					printf("error#must_login_first\n");
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_SUCCESSFUL_LOGIN) {
				if (state == STATE_LOGIN_SENT) {
					printf("login_ack#successful\n");
					
					token = ph_recv->token;
					state = STATE_ONLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
                    state = STATE_OFFLINE;
				}
			} else if (event == EVENT_FAILED_LOGIN) {
                if (state == STATE_LOGIN_SENT) {
                    printf("login_ack#failed\n");

                    state = STATE_OFFLINE;
                } else {
                    send_reset(sockfd, send_buffer, serv_addr);
                    state = STATE_OFFLINE;
                }
            } else if (event == EVENT_FORWARD) {
                if (state == STATE_ONLINE) {
                    char *text = recv_buffer + h_size;
                    printf("%s\n", text);
                } else {
                    send_reset(sockfd, send_buffer, serv_addr);
                    state = STATE_OFFLINE;
                }
            } else if (event == EVENT_POST_ACK) {
				if (state == STATE_POST_SENT) {
					printf("post_ack#successful\n");
					
					state = STATE_ONLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_SUCCESSFUL_SUBSCRIBE_ACK) {
				if (state == STATE_SUBSCRIBE_SENT) {
					printf("subscribe_ack#successful\n");
					
					state = STATE_ONLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event = EVENT_FAILED_SUBSCRIBE_ACK) {
				if (state == STATE_SUBSCRIBE_SENT) {
					printf("subscribe_ack#failed\n");
					
					state = STATE_OFFLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_SUCCESSFUL_UNSUBSCRIBE_ACK) {
				if (state = STATE_UNSUBSCRIBE_SENT) {
					printf("unsubscribe_ack#successful\n");
					
					state = STATE_ONLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_FAILED_UNSUBSCRIBE_ACK) {
				if (state = STATE_UNSUBSCRIBE_SENT) {
					printf("unsubscribe_ack#failed\n");
					
					state = STATE_ONLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_RETRIEVE_ACK) {
				if (state = STATE_RETRIEVE_SENT) {
					char *text = recv_buffer + h_size;
					printf("%s\n", text);
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_RETRIEVE_END_ACK) {
				if (state = STATE_RETRIEVE_SENT) {
					printf("retrieve#successful\n");
					
					state = STATE_ONLINE;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else if (event == EVENT_SUCCESSFUL_LOGOUT) {
				if (state == STATE_LOGOUT_SENT) {
					printf("logout_ack#successful\n");

					state = STATE_OFFLINE;
					event = EVENT_UNKNOWN;
					token = 0;
				} else {
					send_reset(sockfd, send_buffer, serv_addr);
					state = STATE_OFFLINE;
				}
			} else { // event == EVENT_UNKNOWN
				send_reset(sockfd, send_buffer, serv_addr);
				state = STATE_OFFLINE;
			}	
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

int parse_event_from_recv_message(struct header* ph_recv) {
	printf("%x\n", ph_recv->opcode);

	if (ph_recv->opcode == OPCODE_MUST_LOGIN_FIRST) {
		return EVENT_MUST_LOGIN_FIRST;
	} else if (ph_recv->opcode == OPCODE_LOGIN_SUCCESSFUL_ACK) {
		return EVENT_SUCCESSFUL_LOGIN;
	} else if (ph_recv->opcode == OPCODE_LOGIN_FAILED_ACK) {
		return EVENT_FAILED_LOGIN;
	} else if (ph_recv->opcode == OPCODE_FORWARD) {
		return EVENT_FORWARD;
	} else if (ph_recv->opcode == OPCODE_POST_ACK) {
		return EVENT_POST_ACK;
	} else if (ph_recv->opcode == OPCODE_SUBSCRIBE_SUCCESSFUL_ACK) {
		return EVENT_SUCCESSFUL_SUBSCRIBE_ACK;
	} else if (ph_recv->opcode == OPCODE_SUBSCRIBE_FAILED_ACK) {
		return EVENT_FAILED_UNSUBSCRIBE_ACK;
	} else if (ph_recv->opcode == OPCODE_UNSUBSCRIBE_SUCCESSFUL_ACK) {
		return EVENT_SUCCESSFUL_UNSUBSCRIBE_ACK;
	} else if (ph_recv->opcode == OPCODE_UNSUBSCRIBE_FAILED_ACK) {
		return EVENT_FAILED_UNSUBSCRIBE_ACK;
	} else if (ph_recv->opcode == OPCODE_RETRIEVE_ACK)  {
		return EVENT_RETRIEVE_ACK;
	} else if (ph_recv->opcode == OPCODE_RETRIEVE_END_ACK) {
		return EVENT_RETRIEVE_END_ACK;
	} else if (ph_recv->opcode == OPCODE_LOGOUT_ACK) {
		return EVENT_SUCCESSFUL_LOGOUT;
	} else if (ph_recv->opcode == OPCODE_SESSION_RESET) {
		return EVENT_UNKNOWN;
	} else {
		return EVENT_UNKNOWN;
	}
}

void send_reset(int sockfd, char send_buffer[], struct sockaddr_in serv_addr) {
	memset(send_buffer, 0, sizeof(send_buffer));
	send_buffer[0] = MAGIC_NUMBER_1;
	send_buffer[1] = MAGIC_NUMBER_2;
	send_buffer[2] = OPCODE_SESSION_RESET;

	sendto(sockfd, send_buffer, h_size, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
}

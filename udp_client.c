#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
	int connection_status;
	int ret;
	int sockfd = 0;
	char send_buffer[1024];
	char user_input[1024];
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(32000);

	connection_status = connect(sockfd,
		(struct sockaddr *) &serv_addr,
		sizeof(serv_addr));
	if (connection_status < 0) {
		printf("connect() error: %s.\n", strerror(errno));
		return -1;
	}

	while (1) {

		fgets(user_input,
			sizeof(user_input),
			stdin);

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

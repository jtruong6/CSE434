#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[]) {

	int ret;
	int sockfd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
	char recv_buffer[1024];
	char send_buffer[1024];
	socklen_t len;
	int recv_len;
	char recent_msg[201];
	FILE *file;
	file = fopen("log.txt", "w+");
	time_t t;
	char log[1024];

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(32000);

	bind(sockfd,
		(struct sockaddr *) &serv_addr,
		sizeof(serv_addr));

	while (1) {
		len = sizeof(client_addr);
		recv_len = recvfrom(sockfd,
			recv_buffer,
			sizeof(recv_buffer),
			0,
			(struct sockaddr *)&client_addr,
			&len);

		if (recv_len <= 0) {
			printf("recvfrom() error: %s.\n", strerror(errno));
			return -1;
		}

		if (recv_buffer[0] != 'J' || recv_buffer[1] != 'T') {
			//Bad message!!! Skip this iteration.
			continue;
		} else {
			memset(send_buffer, 0, sizeof(send_buffer));
			send_buffer[0] = 'J';
			send_buffer[1] = 'T';

			if (recv_buffer[2] == 1) {	
				memset(recent_msg, 0, sizeof(recent_msg));
				memcpy(recent_msg, recv_buffer+4, recv_buffer[3]);	
				send_buffer[2] = 2;
				send_buffer[3] = 0;

				char message[] = "post_ack#successful";
				memcpy(send_buffer+4, message, sizeof(message));
				
				char timestamp[100];
				time(&t);
				sprintf(timestamp, "%ld", t);

				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

				char client_port[100];
				sprintf(client_port, "%d" , (int) ntohs(client_addr.sin_port));

				char command[206];
				sprintf(command, "post#%s", recent_msg);
				
				sprintf(log, "%s [%s:%s] %s", timestamp, client_ip, client_port, command);
				fputs(log, file);
				fflush(file);
			} else if (recv_buffer[2] == 3) {
				char message[250] = "retrieve_ack#";
				memcpy(message+13, recent_msg, sizeof(recent_msg));

				send_buffer[2] = 4;
				send_buffer[3] = 0;
				memcpy(send_buffer+4, message, sizeof(message));
				
				char timestamp[100];
				time(&t);
				sprintf(timestamp, "%ld", t);

				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

				char client_port[100];
				sprintf(client_port, "%d" , (int) ntohs(client_addr.sin_port));

				char command[206];
				sprintf(command, "retrieve#%s", recent_msg);
				
				sprintf(log, "%s [%s:%s] %s", timestamp, client_ip, client_port, command);
				fputs(log, file);
				fflush(file);
			} else {
				continue;
			}
		}
		
		ret = sendto(sockfd,
			send_buffer,
			sizeof(send_buffer),
			0,
			(struct sockaddr *) &client_addr,
			sizeof(client_addr));

		if (ret <= 0) {
			printf("sendto() error: %s.\n", strerror(errno));
			return -1;
		}
	}
	
	fclose(file);
	close(sockfd);
	return 0;
}

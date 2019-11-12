#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

void *worker_thread(void *arg) {

	int ret;
	int connfd = (int)(unsigned long)arg;
	unsigned char recv_buffer[1024];
	unsigned char name_buffer[1024];
	unsigned char send_buffer[1024];
	FILE *file;

	printf("[connfd-%d] worker thread started.\n", connfd);

	while (1) {
		ret = recv(connfd, recv_buffer, 8, 0);

		if (ret < 0) {
			printf("[connfd-%d] recv() error: %s.\n", connfd, strerror(errno));
			return NULL;
		} else if (ret == 0) {
			printf("[connfd-%d] connection finished\n", connfd);
			break;
		}
		
		if (recv_buffer[0] != 'J' || recv_buffer[1] != 'T') {
			//Bad message, skip
			printf("Wrong magic numbers\n");
			break;
		} else {
			int file_name_length = recv_buffer[3];
			char file_name[file_name_length+1];

			unsigned long file_size = ((unsigned char)recv_buffer[4] << 24) | 
					((unsigned char)recv_buffer[5] << 16) | 
					((unsigned char)recv_buffer[6] << 8) | 
					(unsigned char)recv_buffer[7];

			memset(name_buffer, 0, sizeof(name_buffer));
			ret = recv(connfd, name_buffer, sizeof(name_buffer), 0);
			memset(file_name, 0, sizeof(file_name));
			memcpy(file_name, name_buffer, file_name_length);

			if ((unsigned char)recv_buffer[2] == 0x80) {
				remove(file_name);
				file = fopen(file_name, "wb+");
				if (file == NULL) {
					printf("File empty\n");
					exit(1);
				}
				unsigned long bytes_received = 0;

				while (bytes_received < file_size) {
					memset(recv_buffer, 0, sizeof(recv_buffer));
					ret = recv(connfd, recv_buffer, sizeof(recv_buffer), 0);
					long bytes_to_write = file_size - bytes_received < sizeof(recv_buffer) ? file_size - bytes_received : sizeof(recv_buffer);
					fwrite(recv_buffer, bytes_to_write, 1, file);
					bytes_received += sizeof(recv_buffer);
				}

				fclose(file);
				memset(send_buffer, 0, sizeof(send_buffer));
				send_buffer[0] = 'J';
				send_buffer[1] = 'T';
				send_buffer[2] = 0x81;
				send_buffer[3] = file_name_length;
				send_buffer[4] = 0;
				send_buffer[5] = 0;
				send_buffer[6] = 0;
				send_buffer[7] = 0;
				memcpy(send_buffer+8, file_name, file_name_length);

				ret = send(connfd, send_buffer, sizeof(send_buffer), 0);
				if (ret < 0) {
					printf("send() error: %s.\n", strerror(errno));
					break;
				}
			} else if ((unsigned char)recv_buffer[2] == 0x82) {
				file = fopen(file_name, "rb");
                        	if (file == NULL) {
                                	printf("Error opening file while uploading.\n");
                                	exit(1);
				}

				fseek(file, 0L, SEEK_END);
        	                unsigned long file_size = ftell(file);
	                        rewind(file);

				memset(send_buffer, 0, sizeof(send_buffer));
				send_buffer[0] = 'J';
				send_buffer[1] = 'T';
				send_buffer[2] = 0x83;
				send_buffer[3] = file_name_length;
				send_buffer[4] = (file_size >> 24) & 0xFF;
				send_buffer[5] = (file_size >> 16) & 0xFF;
				send_buffer[6] = (file_size >> 8) & 0xFF;
				send_buffer[7] = file_size & 0xFF;
				
				ret = send(connfd, send_buffer, sizeof(send_buffer), 0);
				if (ret <= 0) {
					printf("send() error: %s.\n", strerror(errno));
					return NULL;
				}

				unsigned long bytes_sent = 0;
				while (bytes_sent < file_size) {
					memset(send_buffer, 0, sizeof(send_buffer));
					long current_payload_size = file_size - bytes_sent < sizeof(send_buffer) ? file_size - bytes_sent : sizeof(send_buffer);
					fread(send_buffer, current_payload_size, 1, file);
					ret = send(connfd, send_buffer, sizeof(send_buffer), 0);
					bytes_sent += current_payload_size;
				}

				fclose(file);
			} else {
				//Unknown OPCODE
				printf("Unknown OPCODE\n");
				continue;
			}
		}

//		printf("[connfd-%d] %s", connfd, recv_buffer);
	}

	printf("[connfd-%d] worker thread terminated.\n", connfd);

	return NULL;
}

int main(int argc, char *argv[]) {
	int ret;
	socklen_t len;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(31000);

	ret = bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	if (ret < 0) {
		printf("bind() error: %s.\n", strerror(errno));
		return -1;
	}

	if (listen(listenfd, 10) < 0) {
		printf("listen() error: %s.\n", strerror(errno));
		return -1;
	}

	while (1) {
		printf("waiting for connection...\n");
		connfd = accept(listenfd, (struct sockaddr*) &client_addr, &len);

		if (connfd < 0) {
			printf("accept() error: %s.\n", strerror(errno));
			return -1;
		}
		printf("connection accept from %s.\n", inet_ntoa(client_addr.sin_addr));

		pthread_t tid;
		pthread_create(&tid, NULL, worker_thread, (void *)(unsigned long)connfd);

	}
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
	int ret;
	int sockfd = 0;
	char user_input[1024];
	unsigned char header_buffer[8];
	unsigned char send_buffer[1024];
	struct sockaddr_in serv_addr;
	FILE *file;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket() error: %s.\n", strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(31000);

	ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (ret < 0) {
		printf("connect() error: %s.\n", strerror(errno));
		return -1;
	}

	while (1) {
		memset(user_input, 0, sizeof(user_input));
		fgets(user_input, sizeof(user_input), stdin);

		if (strncmp(user_input, "exit", strlen("exit")) == 0) {
			shutdown(sockfd, SHUT_RDWR);
			break;
		}

		int file_name_length = 0;
		char file_name[1024];
		if (strncmp(user_input, "upload$", 7) == 0) {
			file_name_length = strlen(user_input)-7-1;

			memset(file_name, 0, sizeof(file_name));
			memcpy(file_name, user_input+7, file_name_length);

			file = fopen(file_name, "rb");
			if (file == NULL) {
				printf("Error opening file while uploading.\n");
				exit(1);
			}

			fseek(file, 0L, SEEK_END);
			unsigned long file_size = ftell(file);
			rewind(file);

			header_buffer[0] = 'J';
			header_buffer[1] = 'T';
			header_buffer[2] = 0x80;
			header_buffer[3] = file_name_length;

			header_buffer[4] = (file_size >> 24) & 0xFF;
			header_buffer[5] = (file_size >> 16) & 0xFF;
			header_buffer[6] = (file_size >> 8) & 0xFF;
			header_buffer[7] = file_size & 0xFF;

			ret = send(sockfd, header_buffer, sizeof(header_buffer), 0);
			if (ret <= 0) {
				printf("send() error: %s.\n", strerror(errno));
				return -1;
			}

			memset(send_buffer, 0, sizeof(send_buffer));
			memcpy(send_buffer, file_name, file_name_length);

			ret = send(sockfd, send_buffer, sizeof(send_buffer), 0);
			if (ret <= 0) {
				printf("send() error: %s.\n", strerror(errno));
				return -1;
			}

			unsigned long bytes_sent = 0;
			while (bytes_sent < file_size) {
				memset(send_buffer, 0, sizeof(send_buffer));
				long current_payload_size = file_size - bytes_sent < sizeof(send_buffer) ? file_size - bytes_sent : sizeof(send_buffer);
				fread(send_buffer, current_payload_size, 1, file);
				ret = send(sockfd, send_buffer, current_payload_size, 0);
				bytes_sent += current_payload_size;
			}

			fclose(file);
		} else if (strncmp(user_input, "download$", 9) == 0) {
			file_name_length = strlen(user_input)-9-1;
			header_buffer[0] = 'J';
			header_buffer[1] = 'T';
			header_buffer[2] = 0x82;
			header_buffer[3] = file_name_length;
			header_buffer[4] = 0;
			header_buffer[5] = 0;
			header_buffer[6] = 0;
			header_buffer[7] = 0;

			memcpy(file_name, user_input+9, file_name_length);

			ret = send(sockfd, header_buffer, sizeof(header_buffer), 0);
			if (ret <= 0) {
				printf("send() error: %s.\n", strerror(errno));
				return -1;
			}

			ret = send(sockfd, file_name, file_name_length, 0);
			if (ret <= 0) {
				printf("send() error: %s.\n", strerror(errno));
				return -1;
			}
		} else {
			// Bad format, skip iteration
			printf("Unknown OPCODE.\n");
			continue;
		}

		char server_response[1024];
		recv(sockfd, server_response, sizeof(server_response), 0);

		if (server_response[0] != 'J' || server_response[1] != 'T') {
			continue;
		} else {
			if ((unsigned char)server_response[2] == 0x81) {
				printf("upload_ack$file_upload_successfully!\n");
			} else if ((unsigned char)server_response[2] == 0x83) {
				unsigned long file_size = ((unsigned char)server_response[4] << 24) |
                                	((unsigned char)server_response[5] << 16) |
                                	((unsigned char)server_response[6] << 8) |
                                	(unsigned char)server_response[7];	
			
				remove(file_name);				
				file = fopen(file_name, "wb+");
				unsigned long bytes_received = 0;
				while (bytes_received < file_size) {
					memset(server_response, 0, sizeof(server_response));
					ret = recv(sockfd, server_response, sizeof(server_response), 0);
					long bytes_to_write = file_size - bytes_received < sizeof(server_response) ? file_size - bytes_received : sizeof(server_response);
					fwrite(server_response, bytes_to_write, 1, file);
					bytes_received += sizeof(server_response);
				}
				
				fclose(file);
				printf("download_ack$file_download_successfully!\n");
			} else {
				//Unknown OPCODE
				printf("Unknown OPCODE.\n");
				continue;
			}
		}
	}

	close(sockfd);

	return 0;
}

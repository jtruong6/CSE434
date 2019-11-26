#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <string>

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
#define STATE_ONLINE 1
#define STATE_MSG_FORWARD 2

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

struct session {
    std::string client_id;
    struct sockaddr_in client_addr;
    time_t last_time;
    uint32_t token;
    int state;

    std::unordered_set<std::string> subbed;
    std::unordered_map<std::string, std::string> subbed_messages;
};

std::unordered_map<u_int32_t, struct session*> all_sessions;
std::unordered_map<std::string, std::string> user_info;

u_int32_t extract_token_from_recv_msg(struct header *ph_recv);
struct session* find_session_by_token(u_int32_t token);
int parse_event_from_recv_msg(struct header *ph_recv);
int authenticate(std::string user_id, std::string password);
int generate_token();

int main() {
    user_info["c1"] = "p1";
    user_info["c2"] = "p2";
    user_info["c3"] = "p3";
    
    srand(time(0));
    int ret;
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    char send_buffer[1024];
    char recv_buffer[1024];
    int recv_len;
    socklen_t len;

    struct session *current_session;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("socket() error: %s.\n", strerror(errno));
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(32000);

    ret = ::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        printf("Binding error!!!");
    }

    struct header *ph_send = (struct header *)send_buffer;
    struct header *ph_recv = (struct header *)recv_buffer;

    while (1) {
        len = sizeof(cli_addr);
        recv_len = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *) &cli_addr, &len);
        if (recv_len <= 0) {
            printf("recvfrom() error: %s.\n", strerror(errno));
            return -1;
        }
        printf("%c\n", recv_buffer[2]);

        u_int32_t token = extract_token_from_recv_msg(ph_recv);
        current_session = find_session_by_token(token);
        int event = parse_event_from_recv_msg(ph_recv);

        if (current_session != NULL) {
            current_session->last_time = time(NULL);
        } else {
            current_session = new session;
        }

        if (event == EVENT_LOGIN) {
            char *id_password = recv_buffer + h_size;
            char *delimiter = strchr(id_password, '&');

            char *pw = delimiter + 1;
            *delimiter = 0;

            char *c_id = id_password;
            delimiter = strchr(pw, '\n');
            *delimiter = 0;

            std::string client_id = std::string(c_id);
            std::string password = std::string(pw);

            ph_send->magic1 = MAGIC_NUMBER_1;
            ph_send->magic2 = MAGIC_NUMBER_2;
            ph_send->payload_length = 0;
            ph_send->msg_id = 0;

            int login_status = authenticate(client_id, password);
            if (login_status > 0) {
                ph_send->opcode = OPCODE_LOGIN_SUCCESSFUL_ACK;
                ph_send->token = (u_int32_t) generate_token();

                current_session->client_id = client_id;
                current_session->state = STATE_ONLINE;
                current_session->token = ph_send->token;
                current_session->last_time = time(NULL);
                current_session->client_addr = cli_addr;

                all_sessions[current_session->token] = current_session;
            } else {
                ph_send->opcode = OPCODE_LOGIN_FAILED_ACK;
                ph_send->token = 0;
            }

            printf("%x\n", send_buffer[2]);
            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
        } 
    }
}

u_int32_t extract_token_from_recv_msg(struct header *ph_recv) {
    return ph_recv->token;
}

struct session* find_session_by_token(u_int32_t token) {
    if (all_sessions.find(token) == all_sessions.end()) {
        return NULL;
    } else {
        return all_sessions.at(token);
    }
}

int parse_event_from_recv_msg(struct header *ph_recv) {
    printf("inside parse event\n");
    if (ph_recv->opcode == OPCODE_LOGIN) {
        printf("inside opcode login\n");
        return EVENT_LOGIN;
    } else if (ph_recv->opcode == OPCODE_POST) {
        return EVENT_POST;
    } else if (ph_recv->opcode == OPCODE_SUBSCRIBE) {
        return EVENT_SUBSCRIBE;
    } else if (ph_recv->opcode == OPCODE_UNSUBSCRIBE) {
        return EVENT_UNSUBSCRIBE;
    } else if (ph_recv->opcode == OPCODE_RETRIEVE) {
        return EVENT_RETRIEVE;
    } else if (ph_recv->opcode == OPCODE_LOGOUT) {
        return EVENT_LOGOUT;
    } else return EVENT_UNKNOWN;
}

int authenticate(std::string user_id, std::string password) {
    if (user_info.find(user_id) == user_info.end()) {
        return 0; // not a valid user
    } else {
        if (user_info.at(user_id).compare(password) == 0) {
            return 1; // valid password
        } else {
            return 0;
        }
    }
}

int generate_token() {
    return rand();
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define MAXLINE 512
#define MAX_SOCK 128

char *escapechar = "exit";
char *join_MSG = "채팅방 접속 성공!\n";

int maxfdp1;
int num_user = 0;
int clisock_list[MAX_SOCK];
int server_sock;

void joinClient(int s, struct sockaddr_in *newclient_addr);
void exitClient(int i);
int set_nonblock(int sockfd);
int is_nonblock(int sockfd);
int sock_listen(int host, int port, int backlog);

void errquit(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	char buf[MAXLINE];
	int i, j, nbyte;
	int accp_sock, client_len;
	struct sockaddr_in client_addr;

	if (argc != 2) {
		printf("사용법 : %s port\n", argv[0]);
			exit(0);
	}

	server_sock = sock_listen(INADDR_ANY, atoi(argv[1]), 5);
	if (server_sock == -1)
		errquit("소켓 리슨 실패");
	if (set_nonblock(server_sock) == -1)
		errquit("논블록 설정 실패");

	printf("참가자 기다리는중...\n");

	while (1) {
		client_len = sizeof(client_addr);
		accp_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
		if (accp_sock == -1 && errno != EWOULDBLOCK)
			errquit("연결 실패");
		else if (accp_sock > 0) {
			clisock_list[num_user] = accp_sock;

			if (is_nonblock(accp_sock) != 0 && set_nonblock(accp_sock) < 0)
				errquit("논블록 설정 실패");

			joinClient(accp_sock, &client_addr);
			send(accp_sock, join_MSG, strlen(join_MSG), 0);
			printf("%d번째 사용자 입장!\n", num_user);
		}

		for (i = 0; i < num_user; i++) {
			errno = 0;
			nbyte = recv(clisock_list[i], buf, MAXLINE, 0);
			if (nbyte == 0) {
				exitClient(i);
				continue;
			}
			else if (nbyte == -1 && errno == EWOULDBLOCK)
				continue;

			if (strstr(buf, escapechar) != NULL) {
				exitClient(i);
				continue;
			}

			buf[nbyte] = 0;

			for (j = 0; j < num_user; j++)
				send(clisock_list[j], buf, nbyte, 0);
			printf("%s\n", buf);
		}
	}

	return 0;
}

void joinClient(int s, struct sockaddr_in *newclient_addr) {
	char buf[20];
	inet_ntop(AF_INET, &newclient_addr->sin_addr, buf, sizeof(buf));
	printf("새로운 참가자의 IP : %s\n", buf);

	clisock_list[num_user] = s;
	num_user++;
}

void exitClient(int i) {
	close(clisock_list[i]);
	if (i != num_user - 1)
		clisock_list[i] = clisock_list[num_user - 1];
	num_user--;
	printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 %d명\n", num_user);
}

int is_nonblock(int sockfd) {
	int value;

	value = fcntl(sockfd, F_GETFL, 0);

	if (value & O_NONBLOCK)
		return 0;
	return -1;
}

int set_nonblock(int sockfd) {
	int value;

	value = fcntl(sockfd, F_GETFL, 0);

	if (fcntl(sockfd, F_SETFL, value | O_NONBLOCK) == -1)
		return -1;
	return 0;
}

int sock_listen(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in server_addr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("소켓 생성 실패");
		exit(1);
	}

	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(host);
	server_addr.sin_port = htons(port);

	if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("바인드 실패");
		exit(1);
	}

	listen(sd, backlog);
	return sd;
}

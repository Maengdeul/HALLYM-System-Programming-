#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>

#define MAXLINE 512
#define MAX_SOCK 128

char *escapechar = "exit";
char name[10];

int main(int argc, char *argv[]) {
	char line[MAXLINE], msg[MAXLINE + 1], checklogin[256] = "Login as",buf[MAXLINE];
	int n, pid;
	struct sockaddr_in server_addr;
	int maxfdp1;
	int s;
	fd_set read_fds;

	if (!(argc == 4 || argc == 5)) {
		printf("사용법 : %s server_IP port name\n", argv[0]);
		exit(0);
	}

	sprintf(name, "[%s]", argv[3]);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("스트림 소켓을 열 수 없음\n");
		exit(0);
	}

	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));

	if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("서버에 접속할 수 없음\n");
		exit(0);
	}
	else {
		printf("서버 접속 성공\n");
		send(s, argv[3], MAXLINE, 0);  //send name
		if(argc == 5) {
			send(s, checklogin, MAXLINE, 0); //send login message
			send(s, argv[4], MAXLINE, 0);  //send password
			recv(s, checklogin, MAXLINE, 0);  //receive login message
			printf("%s\n", checklogin);
		}
		else {
			printf("서버 접속 성공\n");
			send(s, "wrong password",MAXLINE, 0);
			recv(s, checklogin, MAXLINE, 0);
		}
	}

	maxfdp1 = s + 1;
	FD_ZERO(&read_fds);

	while (1) {
		FD_SET(0, &read_fds);
		FD_SET(s, &read_fds);

		if (select(maxfdp1, &read_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0) {
			printf("셀렉트 오류\n");
			exit(0);
		}

		if (FD_ISSET(s, &read_fds)) {
			int size;
			if ((size = recv(s, msg, MAXLINE, 0)) > 0) {
				msg[size] = '\0';
				printf("%s \n", msg);
			}
		}

		if (FD_ISSET(0, &read_fds)) {
			if (fgets(msg, MAXLINE, stdin))
				sprintf(line, "%s %s", name, msg);

			if (send(s, line, strlen(line), 0) < 0)
				printf("소켓 쓰기 에러\n");

			if (strstr(msg, escapechar) != NULL) {
				printf("종료\n");
				close(s);
				exit(0);
			}
		}
	}
}

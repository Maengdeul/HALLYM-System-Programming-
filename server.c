#include <stdio.h>
#include <stdbool.h>
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
#include <openssl/sha.h>

#define MAXLINE 512
#define MAX_SOCK 128

char *escapechar = "exit";
char *join_MSG = "채팅방 접속 성공!\n";

int maxfdp1;
int num_user = 0;
int clisock_list[MAX_SOCK];
int server_sock;

void joinClient(int s, struct sockaddr_in *newclient_addr);
void exitClient(int i, char *namelist, int namecount);
int set_nonblock(int sockfd);
int is_nonblock(int sockfd);
int sock_listen(int host, int port, int backlog);
void RemoveEnd(char *buf);

void errquit(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	char buf[MAXLINE], password[256], checklogin[256] = "Login as", namelist[5][100], getname[50], lines[256];
	int i, j, nbyte, admin[10], admincount = 0, namecount = 0, kicknum = 0;
	int accp_sock, client_len;
	struct sockaddr_in client_addr;
	bool distinguish;
	unsigned char digest[SHA256_DIGEST_LENGTH];

	if (argc != 3) {
		printf("사용법 : %s port passoword\n", argv[0]);
			exit(0);
	}
	
	strcat(password, argv[2]);
	SHA256((unsigned char *)&argv[2], strlen(argv[2]), (unsigned char *)&digest);
	char mdString[SHA256_DIGEST_LENGTH * 2 + 1];
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);

	server_sock = sock_listen(INADDR_ANY, atoi(argv[1]), 5);
	if (server_sock == -1)
		errquit("소켓 리슨 실패");
	if (set_nonblock(server_sock) == -1)
		errquit("논블록 설정 실패");

	printf("서버 비밀번호 : %s\n", mdString);
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
			memset(buf, '\0', sizeof(buf));
			sleep(1);
			recv(clisock_list[num_user - 1], buf, MAXLINE, 0);   //get name
			strcpy(namelist[namecount], buf);  //copy name to namelist
			printf("%d번째 사용자 입장! 이름 : %s\n", num_user, namelist[namecount]);
			namecount++;
			recv(clisock_list[num_user - 1], buf, MAXLINE, 0);  //receive login message
			if (!strcmp(checklogin, buf)) {  //compare with login message when you type password
				recv(clisock_list[num_user - 1], buf, MAXLINE, 0); //receive password 
				if(!strcmp(buf, argv[2])) {
					
                        	        send(clisock_list[num_user - 1], "Now You are Admin!\n", MAXLINE, 0);
                        	        admin[admincount] = num_user - 1;
                        	        admincount++;
                        	        memset(buf, '\0', sizeof(buf));
				}
				else {
					send(clisock_list[num_user - 1], "You are NOT Admin!\n", MAXLINE, 0);
					memset(buf, '\0', sizeof(buf));
				} 
			}
			printf("name list\n");
			for(int j = 0; j < namecount; j++)
				printf("%s\n", namelist[j]);

		}

		for (i = 0; i < num_user; i++) {
			errno = 0;
			nbyte = recv(clisock_list[i], buf, MAXLINE, 0);
			if (nbyte == 0) {
				exitClient(i, namelist, namecount);
				namecount--;
				continue;
			}
			else if (nbyte == -1 && errno == EWOULDBLOCK)
				continue;

			if (strstr(buf, escapechar) != NULL) {
				exitClient(i, namelist, namecount);
				namecount--;
				continue;
			}
			
			char *checkKick = strstr(buf, "/kick ");			
			if(checkKick != NULL) {
				printf("%s type %s\n", namelist[i], checkKick);
				checkKick = strtok(checkKick, " ");
				checkKick = strtok(NULL, " ");
				printf("someone want to kick %s\n", checkKick);
				RemoveEnd(checkKick);
				for(int j = 0; j < num_user; j++) {
					if(admin[j] == i) {
						distinguish = true;
						break;
					}
					else
						distinguish = false;
				}
				if(distinguish) {
					for (int j = 0; j < namecount; j++) {
						if (!strcmp(checkKick, namelist[j])) {
							send(clisock_list[j], "You are kicked by admin.\n", MAXLINE, 0);
							exitClient(j, namelist, namecount-1);
							namecount--;
							printf("name list\n");
		                		        for(int k = 0; k < namecount; k++)
        			                	        printf("%s\n", namelist[k]);

							break;
						}
					}
				}
				else {
					send(clisock_list[i], "You are not admin\n", MAXLINE, 0);
				}
			}
			
			char *checkCommand = strstr(buf, "/list");
			if(checkCommand != NULL) {
				printf("/list detected\n");
				for(j = 0; j < namecount; j++) {
					sprintf(lines, "%s\n", namelist[j]);
					for(int k = 0; k < num_user; k++) {
						send(clisock_list[k], lines, MAXLINE, 0);
					}
				}
			}

			buf[nbyte] = 0;

			for (j = 0; j < num_user; j++) {
				if (send(clisock_list[j], buf, MAXLINE, 0) == -1) {
					break;
				}
			}
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

void exitClient(int i, char *namelist, int namecount) {
	close(clisock_list[i]);
	if (i != num_user - 1)
		clisock_list[i] = clisock_list[num_user - 1];
	for(int j = i ;j > num_user - 1; j++) {
		strcpy(namelist[j], namelist[j+1]);
	}
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

void RemoveEnd(char *buf) {
	int i = 0;
	while (buf[i]) {
		i++;
	}
	buf[i - 1] = '\0';
}

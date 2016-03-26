#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "servermsg.h"
#include "clientmsg.h"

#define FIFO "FIFO"

struct Uname{
	int t;
	char Uusername[20];
};

int sockfd;
char username[20];

struct CLIENTMSG sendMsg, recvMsg;

int main()
{
	int port;
	char ip[20];

	int i;

	//socket
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1){

		perror("socket error");
		exit(1);
	}

	printf("server ip:");
	scanf("%s",ip);
	printf("server port:");
	scanf("%d",&port);

	struct sockaddr_in server;
	bzero(&server,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(port);
	server.sin_addr.s_addr=inet_addr(ip);
	 
	//connect
	int con;
	con=connect(sockfd, (struct sockaddr *)&server, sizeof(server));

	if(con==-1){

		perror("connect error");
		exit(1);
	}
	
	int len=recv(sockfd, &recvMsg, sizeof(recvMsg), 0);

	mkfifo(FIFO ,O_CREAT|O_EXCL);

	if(recvMsg.OP!=OK){

		printf("user is MAX nubber\n");
		close(sockfd);
	}
	else{

		int pid;
		pid=fork();
		if(pid==-1){

			perror("fork error");
			exit(1);
		}
		else if(pid==0){

			struct Uname nm;
			nm.t=0;
			int fd=open(FIFO, O_WRONLY);

			while(1){

				struct CLIENTMSG mes;
				int len=recv(sockfd, &mes, sizeof(mes), 0);

				if(len>0){
					
					if(mes.OP==USER){

						nm.t=0;
						printf("the user %s is login.\n",mes.username );
						write(fd, &nm, sizeof(nm));
					}
					else if(mes.OP == EXIT){

						nm.t=0;
						printf("the user %s is logout.\n",mes.username);
						write(fd, &nm, sizeof(nm));
					}
					else if(mes.OP==NAME){

						nm.t=0;
						printf("online user: %s\n", mes.buf);
						write(fd, &nm, sizeof(nm));
					}
					else if(mes.OP==RENAME){

						printf("%s\n", mes.buf);
						nm.t=1;
						printf("UserName:\n");
						scanf("%s", nm.Uusername);
						sendMsg.OP=USER;
						strcpy(sendMsg.username, nm.Uusername);
						send(sockfd, &sendMsg, sizeof(sendMsg), 0);
						write(fd, &nm, sizeof(nm));
					}
					else if(mes.OP == PUBMSG){

						nm.t=0;
						printf("%s to all: %s\n",mes.username,mes.buf );
						write(fd, &nm, sizeof(nm));
					}
					else if(mes.OP == PRIMSG){

						nm.t=0;
						printf("*private* %s : %s\n",mes.username,mes.buf );
						write(fd, &nm, sizeof(nm));
					}
				}
			}
			exit(0);
		}
		else{
			struct Uname nm;
			nm.t=0;
			int fd=open(FIFO, O_RDONLY);

			printf("UserName:\n");
			scanf("%s", username);

			sendMsg.OP=USER;

			strcpy(sendMsg.username, username);

			send(sockfd, &sendMsg, sizeof(sendMsg), 0);

			while(1){

				char number;
				char b[5]="bye";

				sleep(4);

				read(fd, &nm, sizeof(nm));

				if(nm.t==1){
					strcpy(sendMsg.username, nm.Uusername);
					nm.t=0;
				}
				printf("---------Menu--------\n");
				printf("------1: public------\n");
				printf("------2: private-----\n");
				printf("------3: username----\n");
				printf("------4: exit--------\n");

				printf("input a number\n");
				scanf("%d",&number);

				if(number==1){

					sendMsg.OP=PUBMSG;
					scanf("%s",sendMsg.buf);
					send(sockfd, &sendMsg, sizeof(sendMsg), 0);
				}
				else if(number==2){

					sendMsg.OP=PRIMSG;
					bzero(&sendMsg.toname, sizeof(sendMsg.toname));
					printf("input a name\n");
					scanf("%s",sendMsg.toname);
					scanf("%s",sendMsg.buf);
					send(sockfd, &sendMsg, sizeof(sendMsg), 0);
				}
				else if(number==3){

					sendMsg.OP=NAME;
					send(sockfd, &sendMsg, sizeof(sendMsg), 0);
					
				}
				else if(number==4){

					sendMsg.OP=EXIT;
					send(sockfd, &sendMsg, sizeof(sendMsg), 0);
					break;
				}
				else
					printf("input a right number!\n");

			}

		}

		kill(pid,SIGKILL);
		waitpid(pid, NULL, WNOHANG);
	}
	
	return 0;
}

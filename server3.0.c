#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "clientmsg.h"
#include "servermsg.h"
#include "semaphore.h"

struct client
{
	char username[20];
	char ip[20];
	int port;
	int stat;
	int qid;
}ent[5];


void zhuanfa(int semid);
void tongxin(int connfd,int qid,struct sockaddr_in client);

int main()
{
	int port;
	char ip[20];

	int listenfd, connfd;

	struct sockaddr_in server, client;
	int len=sizeof(client);

	struct CLIENTMSG sendmsg;

	int qid;

	printf("server IP: ");
	scanf("%s", ip);
	printf("server port: ");
	scanf("%d",&port);

	bzero(&server,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(port);
	server.sin_addr.s_addr=inet_addr(ip);

	//socket
	listenfd=socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd==-1){

		perror("socket error");
	}

	//bind
	int bin=bind(listenfd,(struct sockaddr *)&server,sizeof(server));
	if(bin==-1){

		perror("bind error");
		exit(1);
	}

	//listen
	int list=listen(listenfd, 5);
	if(list==-1){

		perror("listen error");
		exit(1);
	}

	key_t key;
	key=ftok(".",'a');
	int semid;
	semid=CreateSem(key,5);

	unlink("SERVER");
	mkfifo("SERVER", O_CREAT);

	int pid=fork();
	int fd=open("SERVER", O_RDONLY|O_NONBLOCK);

	if (pid<0){

		perror("fork1 error");
		exit(1);
	}
	else if (pid==0){
	
		zhuanfa(semid);
		exit(0);
	}
	else{

		while(1){

			connfd=accept(listenfd,(struct sockaddr *)&client,&len);

			int ret=Sem_V(semid);

			if(ret>=0){

				read(fd, &qid, sizeof(qid));

				int pid2=fork();
				if(pid2<0){

					perror("fork2 error");
				}
				else if(pid2==0){

					tongxin(connfd,qid,client);
					exit(0);
				}
				else{

					waitpid(pid2, NULL, WNOHANG);
					close(connfd);
					
					continue;
				}
			}
			else{

				sendmsg.OP=EXIT;
				send(connfd, &sendmsg, sizeof(sendmsg), 0);
				close(connfd);
			}

			while((waitpid(-1, NULL, WNOHANG))>0) 
				;
		}

	}

	close(connfd);
	close(listenfd);
	return 0;
}

void zhuanfa (int semid)
{
	struct SERVERMSG recvmsg; //通信to转发
	struct MESSAGE sendmsg;   //转发to通信

	int i;
	int key;

	for(i=0; i<5; i++){

		ent[i].stat=0;
		key=ftok(".", 102+(char)i);

		ent[i].qid=msgget(key, IPC_CREAT|IPC_EXCL);
		if(ent[i].qid==-1)
		{	
			msgctl(msgget(key,IPC_CREAT), IPC_RMID, NULL);
			ent[i].qid=msgget(key, IPC_CREAT|IPC_EXCL);
		}
		
		printf("%d\n", ent[i].qid);
	}

	int fs=open("SERVER", O_WRONLY|O_NONBLOCK);

	for(i=0; i<5; i++){

		write(fs, &ent[i].qid, sizeof(ent[i].qid));
	}

	unlink("CLIENT");
	mkfifo("CLIENT", O_CREAT);
	int fc=open("CLIENT", O_RDONLY|O_NONBLOCK);

	while(1){

		bzero(&recvmsg, sizeof(recvmsg));

		int len=read(fc, &recvmsg, sizeof(recvmsg));
		if(len>0){

			if(recvmsg.OP==USER){

				for(i=0; i<5; i++){

					if(strcmp(ent[i].username, recvmsg.username)==0){

						break;
					}
				}
				if(i<5){

					sendmsg.msg=recvmsg;
					sendmsg.msgtype=recvmsg.qid;
					sendmsg.msg.OP=RENAME;
					strcpy(sendmsg.msg.buf,"rename!!");
					msgsnd(recvmsg.qid, &sendmsg, sizeof(sendmsg), 0);
				}
				else {

					for(i=0; i<5; i++){

						if(ent[i].qid==recvmsg.qid){

							strcpy(ent[i].username,recvmsg.username);
							strcpy(ent[i].ip,(char *)inet_ntoa(recvmsg.client.sin_addr));
							ent[i].port=ntohs(recvmsg.client.sin_port);
							ent[i].stat=1;
							break;
						}
					}
					write(fc, &recvmsg.qid,sizeof(recvmsg.qid));

					sendmsg.msg=recvmsg;
					sendmsg.msgtype=recvmsg.qid;

					for(i=0; i<5; i++){

						if(ent[i].stat==1){

							msgsnd(ent[i].qid, &sendmsg, sizeof(sendmsg), 0);
						}
					}
				}
			}

			else if(recvmsg.OP==EXIT){

				for(i=0; i<5; i++){

					if(ent[i].qid==recvmsg.qid){

						ent[i].stat=0;
						break;
					}
				}
				Sem_P(semid);
				write(fc, &recvmsg.qid,sizeof(recvmsg.qid));

				sendmsg.msg=recvmsg;
				sendmsg.msgtype=recvmsg.qid;

				for(i=0; i<5; i++){

					if(ent[i].stat==1){

						msgsnd(ent[i].qid, &sendmsg, sizeof(sendmsg), 0);

					}
				}
			}

			else if(recvmsg.OP==NAME){

				sendmsg.msg=recvmsg;
				sendmsg.msgtype=recvmsg.qid;

				for(i=0; i<5; i++){

					if(ent[i].stat==1){

						strcpy(sendmsg.msg.buf,ent[i].username);
						msgsnd(recvmsg.qid, &sendmsg, sizeof(sendmsg), 0);

					}
				}

			}

			else if(recvmsg.OP==PUBMSG){

				sendmsg.msg=recvmsg;
				sendmsg.msgtype=recvmsg.qid;

				for(i=0; i<5; i++){

					if(ent[i].stat==1){

						msgsnd(ent[i].qid, &sendmsg, sizeof(sendmsg), 0);

					}
				}
			}
			
			else if(recvmsg.OP==PRIMSG){

				sendmsg.msg=recvmsg;
				sendmsg.msgtype=recvmsg.qid;
				for(i=0; i<5; i++){

					if(strcmp(ent[i].username,recvmsg.toname)==0){

						msgsnd(ent[i].qid, &sendmsg, sizeof(sendmsg), 0);
						break;
					}			
				}
				if(i==5){

					strcpy(sendmsg.msg.buf,"not user");
					for(i=0;i<5;i++)
					{
						if(strcmp(ent[i].username,recvmsg.username)==0)
						{
							msgsnd(ent[i].qid, &sendmsg, sizeof(sendmsg), 0);
							break;
						}
					}
				}
			}
		}
	}
}

void tongxin(int sockfd,int qid,struct sockaddr_in client)
{
	struct CLIENTMSG recvmsg, sendmsg;

	int fcl;

	fcl=open("CLIENT", O_WRONLY|O_NONBLOCK); 
	int pid=fork();
	if(pid<0){

		perror("tongxin fork error");
		exit(1);
	}
	else if(pid==0){

		bzero(&sendmsg,sizeof(sendmsg));
		sendmsg.OP=OK;
		send(sockfd,&sendmsg,sizeof(sendmsg), 0);

		while(1){

			struct MESSAGE servermsg;

			bzero(&servermsg, sizeof(servermsg));
			bzero(&sendmsg, sizeof(sendmsg));

			if((msgrcv(qid, &servermsg, sizeof(servermsg), 0, 0))==-1)
			{
				perror("msgrcv error");
				exit(1);
			}
			
			sendmsg.OP=servermsg.msg.OP;
			strcpy(sendmsg.buf, servermsg.msg.buf);
			strcpy(sendmsg.username, servermsg.msg.username);
			send(sockfd, &sendmsg, sizeof(sendmsg), 0);
		}
		exit(0);
	}
	else{

		while(1){

			struct SERVERMSG servermsg;

			bzero(&servermsg, sizeof(servermsg));
			bzero(&recvmsg, sizeof(recvmsg));

			int len=recv(sockfd, &recvmsg, sizeof(recvmsg), 0);
			//send(sockfd,&ent,sizeof(ent), 0);

			if(len>0){

				if(recvmsg.OP==USER){

					printf("ip:%s,port:%d,username:%s\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port),recvmsg.username);

					servermsg.OP=recvmsg.OP;
					strcpy(servermsg.buf, recvmsg.buf);
					strcpy(servermsg.username, recvmsg.username);
					servermsg.client=client;
					servermsg.qid=qid;
					int z=write(fcl, &servermsg,sizeof(servermsg));
				}
				else if(recvmsg.OP==EXIT){

					printf("%s exit!\n",recvmsg.username);

					servermsg.OP=recvmsg.OP;
					strcpy(servermsg.buf, recvmsg.buf);
					strcpy(servermsg.username, recvmsg.username);
					servermsg.qid=qid;
					write(fcl, &servermsg,sizeof(servermsg));
					kill(pid,SIGKILL);
					close(sockfd);
					break;
				}
				else if(recvmsg.OP==NAME){

					printf("%s need name\n", recvmsg.username);
					servermsg.OP=recvmsg.OP;
					strcpy(servermsg.buf, recvmsg.buf);
					strcpy(servermsg.username, recvmsg.username);
					servermsg.qid=qid;
					write(fcl, &servermsg,sizeof(servermsg));
				}
				else if(recvmsg.OP==PUBMSG){

					printf("*public*  %s to all: %s\n",recvmsg.username,recvmsg.buf);
					servermsg.OP=recvmsg.OP;
					strcpy(servermsg.buf, recvmsg.buf);
					strcpy(servermsg.username, recvmsg.username);
					servermsg.qid=qid;
					write(fcl, &servermsg,sizeof(servermsg));
				}
				else{

					printf("*private* %s to %s: %s\n",recvmsg.username,recvmsg.toname,recvmsg.buf);
					servermsg.OP=recvmsg.OP;
					strcpy(servermsg.buf, recvmsg.buf);
					strcpy(servermsg.username, recvmsg.username);
					strcpy(servermsg.toname,recvmsg.toname);
					servermsg.qid=qid;
					write(fcl, &servermsg,sizeof(servermsg));
				}
			}
		}
		waitpid(pid, NULL, 0);
	}
}

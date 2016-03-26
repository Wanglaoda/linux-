#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifndef _semaphore
#define _semaphore
union semun
{
    int val; 
    struct semid_ds *buf ;
    unsigned short *array; 
};

int CreateSem(key_t key,int value);
int Sem_P(int semid);
int Sem_V(int semid);

int CreateSem(key_t key,int value)
{
	union semun sem;
	int semid;
	sem.val=value;
	semid=semget(key,1,IPC_CREAT);
	if (semid==-1){
			perror("semget error");	exit(1);
	}
	semctl(semid,0,SETVAL,sem);
	return semid;
}

int Sem_P(int semid)
{
	struct sembuf sops={0,+1,IPC_NOWAIT};
	return (semop(semid,&sops,1));
}

int Sem_V(int semid)
{
	struct sembuf sops={0,-1,IPC_NOWAIT};
	return (semop(semid,&sops,1));
}

#endif

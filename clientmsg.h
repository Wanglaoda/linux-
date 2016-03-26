//CLIENTMSG between server and client
#ifndef _clientmsg
#define _clientmsg

//USER MSG EXIT for OP of CLIENTMSG
#define EXIT -1
#define USER 1
#define PUBMSG 2
#define PRIMSG 3
#define NAME 4
#define RENAME 5
#define OK 6

#ifndef CMSGLEN
#define CMSGLEN 100
#endif

struct CLIENTMSG{
	int OP;
	char username[20];
	char buf[CMSGLEN];
	char toname[20];
};

#endif

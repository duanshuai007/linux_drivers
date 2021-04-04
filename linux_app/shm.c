#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "unistd.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "sys/ipc.h"
#include "sys/shm.h"

#define PERM S_IRUSR|S_IWUSR

int main(int argc,char **argv)
{
	int shmid;
	char *p_addr,*c_addr;
	if(argc != 2)
	{
		fprintf(stderr,"usage:%s\n\a",argv[0]);
		exit(0);
	}
	printf("创建一个新的共享内存\n");
	if((shmid = shmget(IPC_PRIVATE,1024,PERM)) == -1)
	{
			fprintf(stderr,"Create share memory error:%s\n\a",strerror(errno));
			exit(1);
	}
	if(fork())
	{	//在主进程中....
		//获取共享内存的地址
		p_addr = shmat(shmid,0,0);
		memset(p_addr,'\0',1024);
		//将参数传入共享内存中
		strncpy(p_addr,argv[1],1024);
		wait(NULL);
		printf("主进程结束\n");
		exit(0);
	}
	else
	{	//在子进程中....
			sleep(1);
			c_addr = shmat(shmid,0,0);
			printf("Client get %s\n",c_addr);
			exit(0);
	}
}

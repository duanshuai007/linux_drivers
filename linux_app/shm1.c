#include <stdio.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/shm.h>

static char msg[] = "abcdefghijklmnopqrstuvwxyz\n";

int main()
{
	key_t key;
	int shmid,number;
	char i,*shms,*shmc;
	pid_t pid;
	key = ftok("/mnt/user/H",'a');
	shmid = shmget(key,1024,IPC_CREAT|0604);
	pid = fork();
	if(pid > 0)
	{
		shms = shmat(shmid,0,0);
		printf("主进程写入数据\n");
		memcpy(shms,msg,strlen(msg)+1);
		for(number = 1;number <= 5;number++)
		{
			printf("时间过去了%d秒\n",number);
			sleep(1);
		}
	}
	else if(pid==0)
	{
		printf("进入了子进程\n");
		sleep(1);
		shmc = shmat(shmid,0,0);
		printf("子进程读取到的数据为%s\n",shmc);
		shmdt(shmc);
	}
	return 0;
}

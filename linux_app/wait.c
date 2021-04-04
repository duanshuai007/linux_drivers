#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	pid_t child_pt;
	int number;
	child_pt = fork();
	if(child_pt < 0)
	{
		printf("创建子进程失败\n");
		exit(1);
	}
	else if(!child_pt)
	{
		printf("进入子进程\n");
		printf("子进程PID = %d\n",getpid());
		for(number = 0;number < 10;number++)
		{
			printf("子进程打印次数增加到%d!\n",number+1);
		}
		printf("子进程结束\n");
	}
	else
	{
		printf("进入主进程\n");
		printf("主进程PID = %d\n",getpid());
		printf("等待子进程结束\n");
		wait(NULL);
		printf("主进程结束\n");
	}
}

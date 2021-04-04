#include "stdio.h"
#include "sys/types.h"
#include "signal.h"
#include "stdlib.h"
#include "sys/wait.h"

int main()
{
	pid_t pid;

	pid = fork();
	if(pid < 0)
		printf("Create fork failed\n");
	if(pid == 0)
	{
		printf("Enter child process\n");
		printf("chile(pid:%d)is waiting for any signal\n",getpid());
		raise(SIGSTOP);
		exit(0);
	}
	else
	{
		printf("Enter parent process\n");
		sleep(1);
		if(waitpid(pid,NULL,WNOHANG) == 0)
		{
			if(kill(pid,SIGKILL) == 0)
			{
				printf("kill %d",pid);
			}
		}
		waitpid(pid,NULL,0);
		exit(0);
	}
}

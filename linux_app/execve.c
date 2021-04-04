#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

int main()
{
	int i;
	int status;
	pid_t pid,childpid;
	if((childpid = fork()) == 0)
	{
		execve("/mnt/user/H",NULL,environ);
	}
	if((pid = getpid()) != 0)
	{
		//wait(&childpid);
		waitpid(childpid,&status,0);
		for(i=0;i<10;i++)
		{
			printf("i = %d\t",i);
		}
		printf("in process\n");
	}
	return 0;
}

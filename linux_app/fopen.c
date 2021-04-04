#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "fcntl.h"
#define BUFSIZE 1024

int main()
{
	FILE *fp;
	char *cmd = "ps -ef";
	char buf[BUFSIZE];

	if((fp = popen(cmd,"r")) == NULL)
	{
		printf("popen error\n");
		exit(1);
	}
	while((fgets(buf,BUFSIZE,fp)) != NULL)
	{
		printf("%s",buf);
	}
	pclose(fp);
	exit(0);
}

#include "sys/types.h"
#include "sys/stat.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include "fcntl.h"
#include "string.h"

#define FIFO "/tmp/myfifo"

int main(int argc,char **argv)
{
	int fd;
	int w_num;
	char w_buf[100];
	if(argc != 2)
	{
		printf("Please input something\n");
		fgets(w_buf,100,stdin);
	}
	else
		strcpy(w_buf,argv[1]);

	fd = open(FIFO,O_WRONLY|O_NONBLOCK,0);
	if(fd == -1)
	{
		printf("Open Failed\n");
		exit(-1);
	}
	if((w_num = write(fd,w_buf,100)) == -1)
	{	//这里的错误判断是必需的，不然就会无法发送
		if(errno == EAGAIN)
			printf("Write Error\n");
	}
	else 
		printf("write %s to the fifo\n",w_buf);
	//close(fd);
}

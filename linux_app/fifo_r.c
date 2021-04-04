#include "sys/types.h"
#include "sys/stat.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "fcntl.h"
#include "errno.h"

#define FIFO "/tmp/myfifo"

int main()
{
	int fd;
	int r_num;
	char r_buf[100];
	//access用来检测是否可以读写某一已存在的文件，F_OK是用来检测文件是否已存在的参数
	if(access(FIFO,F_OK) == -1)	//如果不存在则创建fifo
	{	
		if((mkfifo(FIFO,O_CREAT|O_EXCL) < 0)&&(errno != EEXIST))
		{	//if中的错误判断也是必需的，不然就会失败
			fprintf(stderr,"can't create fifo %s\n",FIFO);
			exit(-1);
		}
	}
	fd = open(FIFO,O_RDONLY|O_NONBLOCK,0);
	if(fd == -1)
	{
		perror("open");
		exit(1);
	}
	while(1)
	{
			memset(r_buf,0,sizeof(r_buf));
			r_num = read(fd,r_buf,100);
			//if(r_num == 0) printf("can't read\n");
			if(r_num > 0) printf("read %s form fifo",r_buf);
			printf("wait for...\n");
			sleep(1);
	}
}

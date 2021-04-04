#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc,char *argv[])
{
	int from_fd,to_fd;
	int read_bytes,write_bytes;
	char buf[BUFFER_SIZE];
	char *ptr;
	if(argc != 3)
	{
		fprintf(stderr,"USAGE:%s fromfile to file\n",argv[0]);
		exit(1);
	}
	if((from_fd = open(argv[1],O_RDONLY)) == -1)
	{
		fprintf(stderr,"open %s error",argv[1]);
		exit(1);
	}
	if((to_fd = open(argv[2],O_RDWR|O_CREAT,S_IWUSR|S_IRUSR)) == -1)
	{
		fprintf(stderr,"open %s error",argv[2]);
		exit(1);
	}
	while(read_bytes = read(from_fd,buf,BUFFER_SIZE))
	{
		if(read_bytes == -1)
		{
			perror("read");exit(1);
		}
		else
		{
			if(read_bytes > 0)
			{
				ptr = buf;
				while(write_bytes = write(to_fd,ptr,read_bytes))
				{
					if(write_bytes == -1)
					{
						perror("wirte");exit(1);
					}
					else 
						if(write_bytes == read_bytes)
						{
							break;
						}
						else
							if(write_bytes > 0)
							{
								ptr += write_bytes;
								read_bytes -= write_bytes;
							}		
				}
			}
		}
	}
	close(from_fd);
	close(to_fd);
	exit(0);
}

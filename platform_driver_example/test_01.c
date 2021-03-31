#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TYPE 'D'

int main(int argc, char *argv[])
{
    int fd, ret;
    int freq, pin;
    char str[32];

	if (argc < 3) {
		printf("run like this\r\n");
		printf("./test1 [pin] [freq]\r\n");
		printf("sudo ./test1 0 5\r\n");
		exit(1);
	}

    fd = open("/dev/platform_test_driver", O_RDONLY);
    if(fd < 0) {
        printf("open led error\r\n");
        exit(1);
    }

	//printf("sleep 2 second!\r\n");
	//sleep(2);
	//printf("will ioctl\r\n");
    
	pin = atoi(argv[1]);
	freq = atoi(argv[2]);
    printf("freq:%d\r\n", freq);
    
	ret = ioctl(fd, _IOWR(TYPE,5,int), (pin << 16) | freq);
    
	while(1) {
        ret = read(fd, str, sizeof(str));
        printf("read:ret=%d\r\n",ret);
		printf("read:%s\r\n", str);
		sleep(1);
    }

    return 0;
}



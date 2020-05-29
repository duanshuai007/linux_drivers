#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>

#define TYPE 'D'
int main(int argc, char *argv[])
{
    int fd, ret;
    int cmd, arg;
    if(argc < 3)
    {
        printf("argc num error\r\n");
        exit(1);
    }
    fd = open("/dev/s3c2440_led", O_RDONLY);
    if(fd < 0)
    {
        printf("open led error\r\n");
        exit(1);
    }
    arg = atoi(argv[2]);
    cmd = atoi(argv[1]);
    printf("argv[1]:%d,argv[2]:%d\r\n",arg,cmd);
    ret = ioctl(fd, _IOWR(TYPE,cmd,int), arg);
    printf("ret=%d\r\nend\r\n",ret);
    while(1)
    {
        sleep(1);
        printf("main while\r\n");
    }
    return 0;
}

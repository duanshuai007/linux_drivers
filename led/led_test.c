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
    int freq;
    char str[20];
    fd = open("/dev/s3c2440_led", O_RDONLY);
    if(fd < 0)
    {
        printf("open led error\r\n");
        exit(1);
    }
    freq = atoi(argv[1]);
    printf("freq:%d\r\n", freq);
    ret = ioctl(fd, _IOWR(TYPE,5,int), freq);
    while(1)
    {
        ret = read(fd,str,sizeof(str));
        printf("main while %d\r\n",ret);
    }
    return 0;
}

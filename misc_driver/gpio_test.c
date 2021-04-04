#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/stat.h>

#define TYPE 'D'
int main(void)
{
    int fd;

    fd = open("/dev/DS_GPIO", O_RDWR);
    if(fd < 0)
    {
        printf("open gpio failed\r\n");
        return -1;
    }
    while(1)
    {
        ioctl(fd, _IOWR(TYPE, 3, int), 1);
        sleep(1);
        ioctl(fd, _IOWR(TYPE, 3, int), 0);
        sleep(1);
    }
    return 0;
}

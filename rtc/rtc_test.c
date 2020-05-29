#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    int fd, ret;
    char timebuffer[20];

    fd = open("dev/s3c2440_rtc", 0);
    if(fd < 0)
    {
        printf("main:open rtc failed\r\n");
        exit(1);
    }
    while(1)
    {
        ret = read(fd, timebuffer, 20);
        sleep(1);
        printf("main running...%s\r\n",timebuffer);
    }
}


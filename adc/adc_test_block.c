#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

int main(int argc, char * argv)
{
    int fd, ret, flag = 0;
    fd_set fds;
    char dat[20];
    fd = open("/dev/s3c2440_adc", 0);
    if(fd < 0)
    {
        printf("open adc failed\r\n");
        exit(1);
    }
    printf("main adc open\r\n");
    while(1)
    {
        ret = read(fd, &dat, sizeof(dat));
        if(ret > 0)
        {
        //    printf("dat:%s,ret:%d\r\n", dat, ret);
        }
        else{
            //printf("Read ADC Failed\r\n");
        }
    }
    close(fd);
    return 0;
}

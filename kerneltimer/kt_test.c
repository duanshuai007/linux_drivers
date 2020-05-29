#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

int main(void)
{
    int fd;
    int counter = 0;
    int old_counter = 0;

    fd = open("/dev/kerneltimer", O_RDONLY);
    if(fd < 0)
    {
        printf("can't open second\r\n");
        exit(1);
    }
    while(1)
    {
        read(fd, &counter, sizeof(unsigned int));
        if(counter != old_counter)
        {
            printf("seconds after open /dev/kerneltimer:%d\r\n", counter);
            old_counter = counter;
        }
        printf("read count:%d\r\n", counter);
    }
    return 0;
}

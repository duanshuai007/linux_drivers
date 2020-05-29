#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>

int main()
{
    int button_fd;
    int key_value;

    button_fd = open("/dev/s3c2440_button", O_NONBLOCK);

    if(button_fd < 0)
    {
        perror("open");
        exit(1);
    }

    while(1)
    {
        int ret;
            int readret = read(button_fd, &key_value, sizeof(key_value));
            if(readret != sizeof(key_value))
            {
                //printf("not read key\r\n");
            }
            else{
                printf("button value:%d\r\n", key_value);
            }
    }
    close(button_fd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

int main()
{
    int button_fd;
    int key_value;

    button_fd = open("/dev/s3c2440_button", 0);

    if(button_fd < 0)
    {
        perror("open");
        exit(1);
    }

    while(1)
    {
        fd_set rds;
        int ret;

        FD_ZERO(&rds);
        FD_SET(button_fd,&rds);
        ret = select(button_fd+1, &rds, NULL, NULL, NULL);
        if(ret < 0)
        {
            perror("select");
            exit(1);
        }
        if(ret == 0)
        {
            printf("Timeout...\r\n");
        }else if(FD_ISSET(button_fd, &rds))
        {
            int readret = read(button_fd, &key_value, sizeof(key_value));
            if(readret != sizeof(key_value))
            {
                if(errno != EAGAIN)
                {
                    perror("read button\r\n");
                }
                continue;
            }
            else{
                printf("button value:%d\r\n", key_value+1);
            }
        }
    }
    close(button_fd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define TEST_IOC_MAGIC 'k'
#define TEST_IOC_MAXNR 14
#define TEST_GET_VERSION 1
#define TEST_SET_VERSION 2

#if 0
struct test {
    char i;
    int j;
    int k;
};

int main()
{

    struct test t1;

    printf("&t1=%d\r\n", &t1);
    printf("&t1.k=%d\r\n", &t1.k);
    printf("&((struct test *)0)-> = %d\r\n", ((int)&((struct test *)0)->k));
}
#else

#define MESSAGE "hello duanshuai!"

int main(int argc, char *argv[])
{
    int fd, k;
    int len = 0;
    char buff[32] = {0};

    if (argc < 3) {
        printf("paramter error:%d\r\n", argc);
        return 0;
    }

    int pos = atoi(argv[1]);
    int count = atoi(argv[2]);
    printf("pos=%d,count=%d\r\n", pos, count);

    fd = open("/dev/aTest", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 0;
    }

    printf("open success\r\n");

    int version = 20;

    ioctl(fd, _IOW(TEST_IOC_MAGIC, TEST_SET_VERSION, int), &version);
    version = 0;
    ioctl(fd, _IOW(TEST_IOC_MAGIC, TEST_GET_VERSION, int), &version);
    printf("version=%d\r\n", version);
    //ioctl(fd,  1, NULL);

#if 1
   int i; 
    for(i=pos;i<(pos + count);i++) {
        //lseek(fd, SEEK_SET, i * strlen(MESSAGE));
        len = write(fd, MESSAGE, strlen(MESSAGE));
        if (len < 0) {
            perror("write");
        } else {
            printf("write len=%d\r\n", len);
        }
    }
#endif

    close(fd);

    return 1;
}

#endif


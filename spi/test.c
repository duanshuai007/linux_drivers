#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <linux/ioctl.h>

#define TYPE 'D'

#define PIN_CS      _IOWR(TYPE, 3, int)
#define PIN_RESET   _IOWR(TYPE, 0, int)
#define TRANS_CMD   _IOWR(TYPE, 10, int)
#define TRANS_DATA  _IOWR(TYPE, 11, int)

#define msleep(x) usleep(x*1000)

int main()
{
    int fd;
    char buffer[16];
    int len;

    fd = open("/dev/s3c2440_spi", O_RDWR);
    if(fd < 0)
    {
        printf("open spi failed\r\n");
        return -1;
    }

    ioctl(fd, PIN_CS, 0);
    ioctl(fd, PIN_RESET, 1);
    msleep(50);
    ioctl(fd, PIN_RESET, 0);
    sleep(1);
    ioctl(fd, PIN_RESET, 1);
    sleep(1);
    //start initial sequence
    buffer[0] = 0x11;
    write(fd, buffer, 1);
    msleep(20);
/*
    buffer[0] = 0xd0;
    buffer[1] = 0x07;
    buffer[2] = 0x41;
    buffer[3] = 0x1f;
    write(fd, buffer, 4);

    buffer[0] = 0xd1;
    buffer[1] = 0x00;
    buffer[2] = 0x20;
    buffer[3] = 0x0d;
    write(fd, buffer, 4);

    buffer[0] = 0xd2;
    buffer[1] = 0x03;
    buffer[2] = 0x00;
    write(fd, buffer, 3);

    buffer[0] = 0xc0;
    buffer[1] = 0x10;
    buffer[2] = 0x3b;
    buffer[3] = 0x00;
    buffer[4] = 0x02;
    buffer[5] = 0x11;
    write(fd, buffer, 6);

    buffer[0] = 0xc5;
    buffer[1] = 0x02;
    write(fd, buffer, 2);

    buffer[0] = 0xc8;
    buffer[1] = 0x00;
    buffer[2] = 0x01;
    buffer[3] = 0x20;
    buffer[4] = 0x01;
    buffer[5] = 0x10;
    buffer[6] = 0x0f;
    buffer[7] = 0x74;
    buffer[8] = 0x67;
    buffer[9] = 0x77;
    buffer[10] = 0x50;
    buffer[11] = 0x0f;
    buffer[12] = 0x10;
    write(fd, buffer, 13);

    buffer[0] = 0xf8;
    buffer[1] = 0x01;
    write(fd, buffer, 2);

    buffer[0] = 0xfe;
    buffer[1] = 0x00;
    buffer[2] = 0x02;
    write(fd, buffer, 3);

    buffer[0] = 0xb4;
    buffer[1] = 0x11;
    write(fd, buffer, 2);

    buffer[0] = 0x2a;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x01;
    buffer[4] = 0x3f;
    write(fd, buffer, 5);

    buffer[0] = 0x2b;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x01;
    buffer[4] = 0xdf;
    write(fd, buffer, 5);

    buffer[0] = 0x3a;
    buffer[1] = 0x66;
    write(fd, buffer, 2);

    buffer[0] = 0x36;
    buffer[1] = 0x0a;
    write(fd, buffer, 2);

    msleep(200);
    buffer[0] = 0x29;
    write(fd, buffer, 1);

    buffer[0] = 0x2c;
    write(fd, buffer, 1);
*/
    while(1)
    {
        //ioctl(fd, PIN_CS, 0);
        //ioctl(fd, PIN_RESET, 0);
        ioctl(fd, TRANS_CMD, 1);
        buffer[0] = 0xbf;
        write(fd, buffer, 1);
        memset(buffer, 0, 6);
        for(int i = 0; i < 6; i++)
        {
            ioctl(fd, TRANS_DATA, 1);
            read(fd, buffer[i], 1);
        }
        /*buffer[0] = 0xbf;
        write(fd, buffer, 1);
        read(fd, buffer, 6);*/
        printf("read:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",
                buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

        sleep(1);
        //ioctl(fd, PIN_CS, 1);
        //ioctl(fd, PIN_RESET, 1);
        //ioctl(fd, TRANS_DATA, 0x55);
        //sleep(1);
        //printf("init complete\r\n");
    }

    close(fd);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

struct eeprom_data{
    unsigned char addr;
    unsigned char data;
}ed;

#define I2C_READ    0X100001
#define I2C_WRITE   0X100002

int main(void)
{
    int fd, ret;
    char buf[1024];
    int len;
    int i;

    fd = open("/dev/at24c08", O_RDWR);
    if(fd < 0)
    {
        printf("open at24c02 failed \r\n");
        exit(1);
    }

    printf("start write\r\n");

    for(i=0; i< 256; i++)
    {
        buf[i] = i;
    }
    for(i=0; i< 256; i++)
    {
        buf[i+256] = i*2;
    }
    for(i=0;i<256;i++)
    {
        buf[i+512] = i*3;
    }
    for(i=0;i<256;i++)
    {
        buf[i+512+256] = i*4;
    }

    lseek(fd, 0, SEEK_SET);

    write(fd, buf, 1024);

    printf("i2c write over\r\n");

    for(i=0; i < 1024; i++)
    {
        buf[i] = 0;
    }

    lseek(fd, 0, SEEK_SET);

    len = read(fd, buf, 1024);
    printf("read:len=%d\r\n", len);
    for(i=0;i<1024;i++)
    {
        printf("%02x ",buf[i]);
        if((i+1)%16 == 0) printf("\r\n");
        if((i+1)%256 == 0) printf("-----------------------------------\r\n");

    }
    printf("\r\n");
    close(fd);

    return 0;
}

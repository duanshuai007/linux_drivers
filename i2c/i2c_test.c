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
    char buf[10];
    int len;
    int i;

    fd = open("/dev/at24c02", O_RDWR);
    if(fd < 0)
    {
        printf("open at24c02 failed \r\n");
        exit(1);
    }

    printf("start write\r\n");

    for(i=0; i< 128; i++)
    {
        usleep(10);
        ed.addr = i;
        ed.data = i*2;
        if(ioctl(fd, I2C_WRITE, &ed) < 0)
        {
            printf("i2c write error\r\n");
            exit(1);
        }
    }

    printf("i2c write over\r\n");

    for(i=0; i < 128; i++)
    {
        usleep(10);
        ed.addr = i;
        ed.data = 0;
        if(ioctl(fd, I2C_READ, &ed) < 0)
        {
            printf(" i2c read error\r\n");
            exit(1);
        }
        printf("0x%02x ", ed.data);
        if((i+1) % 8 == 0) printf("\r\n");
    }
    printf("\r\n");
    close(fd);
    return 0;
}

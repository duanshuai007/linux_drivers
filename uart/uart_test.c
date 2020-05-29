#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if( tcgetattr( fd, &oldtio ) != 0)
    {
        perror("Setupserial 1");
        exit(1);
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch( nBits )
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
        default:
            printf("serial nBits error\r\n");
            return -1;
    }
    switch(nEvent)
    {
        case 'o':
        case 'O':
            {
                newtio.c_cflag |= PARENB;
                newtio.c_cflag |= PARODD;
                newtio.c_iflag |= (INPCK | ISTRIP);
            }break;
        case 'e':
        case 'E':
            {
                newtio.c_iflag |= (INPCK | ISTRIP);
                newtio.c_cflag |= PARENB;
                newtio.c_cflag &= ~PARODD;
            }break;
        case 'n':
        case 'N':
            {
                newtio.c_cflag &= ~PARENB;
            }break;
        default:
            printf("serial nEvent error\r\n");
            return -1;
    }

    switch(nSpeed)
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        default:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
    }

    switch(nStop)
    {
        case 1:
            newtio.c_cflag &= ~CSTOPB;
            break;
        case 2:
            newtio.c_cflag |= CSTOPB;
            break;
        default:
            printf("serial nStop error\r\n");
            return -1;
    }

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIFLUSH);

    if((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        perror("com set error\r\n");
        return -1;
    }
    printf("set done!\r\n");
    return 0;
}

int open_port(int fd, int comport)
{
    char *dev[] = {"dev/ttySAC0", "dev/ttySAC1", "dev/ttySAC2"};
    long vdisable;
    printf("ready open %s\r\n", dev[comport]);
    fd = open(dev[comport], O_RDWR|O_NOCTTY|O_NDELAY);
    if(fd < 0)
    {
        perror("can't open serial");
        return -1;
    }
    printf("open serial success\r\n");
    if(fcntl(fd, F_SETFL, 0) < 0)
    {
        printf("fcntl failed\r\n");
    }else{
        printf("fcntl=%d\r\n", fcntl(fd, F_SETFL, 0));
    }

    if(isatty(STDIN_FILENO) == 0)
    {
        printf("standard input is not a terminal device\r\n");
    }else{
        printf("isatty success\r\n");
    }

    return fd;
}

int main(int argc, char **argv)
{
    int fd;
    int nread, i;
    int serial_no;
    char buff[] = "Hello\r\n";

    if(argc < 2)
    {
        printf("please input serial number:\r\n");
    }
    serial_no = atoi(argv[1]);
    printf("serial no:%d\r\n", serial_no);
    if((fd = open_port(fd, serial_no)) < 0)
    {
        perror("open_port error");
        return 1;
    }
    if((i = set_opt(fd, 115200, 8,'n', 1)) < 0)
    {
        perror("set_opt error");
        return 1;
    }
    printf("fd = %d\r\n", fd);

    while(1)
    {
        write(fd, "hello world\r\n", sizeof("hello world\r\n"));
        sleep(1);
    }
    nread = read(fd, buff, 8);
    printf("nread=%d, %s\r\n", nread, buff);
    close(fd);
}

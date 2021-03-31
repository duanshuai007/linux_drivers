#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

static char buf[256] = {1};
#define LCD_DRV_MAGIC               'K'
#define LCD_CMD_MAKE(cmd)           (_IO(LCD_DRV_MAGIC, cmd))
#define LCD_OPEN_SCREEN				6
#define LCD_CLOSE_SCREEN			7

int main(int argc,char *argv[])
{
	int fd = open("/dev/kobject_test_driver",O_RDWR);
	int ret;
	int pinstate;
	char buffer[32];

	if(fd < 0) {
		perror("Open file failed!!!\r\n");
		return -1;
	}
	
	while(1){
#if 0
		ioctl(fd, LCD_CMD_MAKE(LCD_CLOSE_SCREEN), 0);
		sleep(5);
		ioctl(fd, LCD_CMD_MAKE(LCD_OPEN_SCREEN), 0);
		sleep(5);
#endif
		write(fd, "on", 2);
		sleep(2);
		memset(buffer, 0, sizeof(buffer));
		read(fd, buffer, sizeof(buffer));
		printf("read:%s\r\n", buffer);
		
		write(fd, "off", 3);
		sleep(2);
		memset(buffer, 0, sizeof(buffer));
		read(fd, buffer, sizeof(buffer));
		printf("read:%s\r\n", buffer);
	}

	close(fd);

	return 0;
}

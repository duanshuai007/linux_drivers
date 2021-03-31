#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <netdb.h>


#define LCD_DRV_MAGIC               'K'
#define LCD_CMD_MAKE(cmd)           (_IO(LCD_DRV_MAGIC, cmd))
#define LCD_WINDOW_START		3
#define LCD_WINDOW_CONTINUE		4
#define LCD_WINDOW_END			5
#define LCD_CLOSE_SCREEN		7
#define LCD_RESET				99

//160行每行162列
//每个点阵用4bit表示，0xf 或 0x0
//所以表示一列的数组大小就是162 / 2

#define MAXEVENTS	64
static int socket_fd = -1;
static int epoll_fd = -1;
static unsigned char display_buffer[160 * 81];
static unsigned char buffer[160 * 160];
static int read_pos = 0;
static int read_total_size = 0;

#define  DEBUG		0

static void process_new_data(int fd, int devfd) 
{
	ssize_t count;
	int i, j;
	int pos, val;
	int ret;

	while ((count = read(fd, buffer + read_pos, sizeof(buffer) - read_pos))) {
	
		if (count == -1) {
			/* EAGAIN, read all data */
			if (errno == EAGAIN)
				return;

			perror("read");
			break;
		}

		//printf("read_total_size=%d count = %d strlen(close)=%d read_pos=%d\n", read_total_size, count, strlen("close"), read_pos);
		if ((count == 5) && (read_pos == 0)) {
			if (strncmp(buffer, "close", strlen("close")) == 0) {
				//printf("LCD_CLOSE_SCREEN!\n");
				ret = ioctl(devfd, LCD_CMD_MAKE(LCD_CLOSE_SCREEN), 0);
				//printf("ioctl ret = %d\n", ret);
				break;
			}
		}

		read_total_size += count;
		//printf("count = %d\n", count);
		if (read_total_size != sizeof(buffer)) {
			read_pos = count;
			continue;
		} else {
			read_total_size = 0;
			read_pos = 0;
		}

		memset(display_buffer, 0, sizeof(display_buffer));
		for(i = 0; i < 160; i++) {
			pos = 0;
			for (j = 0; j < 160; j++) {
				if ((j!=0) && (j%2 == 0))
					pos++;
				val = buffer[i*160 + j];
				if (j%2 != 0)
					display_buffer[i*81 + pos] |= val;
				else
					display_buffer[i*81 + pos] |= (val << 4);					
			}
		}
#if DEBUG
		for (i = 0; i < 160; i++) {
			for (j = 0; j < 81; j++) {
				printf("%02x", display_buffer[i*81 +j]);
			}
			printf("\n");
		}
#else
		ioctl(devfd, LCD_CMD_MAKE(LCD_WINDOW_START), 162);
		for (i = 0; i < 80; i++) {
			if ( ioctl(devfd, LCD_CMD_MAKE(LCD_WINDOW_CONTINUE), &display_buffer[i * 162]) < 0) {
				//do something
			}
		}
		ioctl(devfd, LCD_CMD_MAKE(LCD_WINDOW_END), 0);
#endif
	}
}

static int spilcd_on(void)
{
	int fd;
#if DEBUG
	fd = 32;
#else
	fd = open("/dev/spilcd", O_RDWR);
	if(fd < 0) {
		printf("open /dev/spilcd failed\n");
		return -1;	
	}
#endif
	return fd;
}

/*
 *		输入参数为1维list
 *		LCD屏幕每行有160个像素点
 *		每个像素点用4bit来表示
 *		每行的寻址范围是0-162，其中超出160的点位不显示在屏幕上，但是在内存中必须要有占位
 */

/* epoll example */
static int make_socket_non_blocking(int socket_fd)
{   
	int flags, s;

	flags = fcntl(socket_fd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK; 
	s = fcntl(socket_fd, F_SETFL, flags);
	if (s == -1) {
		perror("fcntl");
		return -1;
	}

	return 0;
}

static int socket_create_bind_local(int port)
{   
	struct sockaddr_in server_addr;
	int opt = 1;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}

	if (setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int)) == -1) {
		perror("Setsockopt");
		exit(1);
	}

	server_addr.sin_family = AF_INET;  
	server_addr.sin_port = htons(port);    
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero),8);

	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		return -1;
	}

	return 0;
}

static void accept_and_add_new()
{   
	struct epoll_event event;
	struct sockaddr in_addr;
	socklen_t in_len = sizeof(in_addr);
	int infd;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	while ((infd = accept(socket_fd, &in_addr, &in_len)) != -1) {

		if (getnameinfo(&in_addr, in_len,
					hbuf, sizeof(hbuf),
					sbuf, sizeof(sbuf),
					NI_NUMERICHOST | NI_NUMERICHOST) == 0) {
			printf("Accepted connection on descriptor %d (host=%s, port=%s)\n",
					infd, hbuf, sbuf);
		}
		/* Make the incoming socket non-block
		 * and add it to list of fds to
		 * monitor*/
		if (make_socket_non_blocking(infd) == -1) {
			abort();
		}

		event.data.fd = infd;
		event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, infd, &event) == -1) {
			perror("epoll_ctl");
			abort();
		}
		in_len = sizeof(in_addr);
	}

	if (errno != EAGAIN && errno != EWOULDBLOCK) {
		perror("accept");
	}
	/* else
	 *
	 * We hae processed all incomming connectioins
	 *
	 */
}

int main(int argc, char *argv[])
{
	struct epoll_event event;
	struct epoll_event *events;
	int port;
	int device_fd;

	if (argc < 2) {
		printf("paramter too low!\n");
		return -1;
	}

	port = atoi(argv[1]);

	device_fd = spilcd_on();
	if (device_fd < 0) {
		printf("open spilcd failed\n");
		return -1;
	}

	socket_create_bind_local(port);

	if (make_socket_non_blocking(socket_fd) == -1) {
		printf("make_socket_non_blocking error\n");
		exit(1);
	}

	if (listen(socket_fd, 10) == -1) {
		perror("Listen");
		exit(1);
	}

	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		perror("epoll_create1");
		exit(1);
	}

	event.data.fd = socket_fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
		perror("epoll_ctl");
		exit(1);
	}

	events = calloc(MAXEVENTS, sizeof(event));

	while(1)
	{
		int n, i;
		n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
		for (i = 0; i < n; i++) {
			if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
					!(events[i].events & EPOLLIN)) {
				/* An error on this fd or socket not ready */
				perror("epoll error");
				close(events[i].data.fd);
			} else if (events[i].data.fd == socket_fd) {
				/* New incoming connection */
				accept_and_add_new();
			} else {
				/* Data incoming on fd */
				process_new_data(events[i].data.fd, device_fd);
			}
		}
	}

	free(events);
	close(socket_fd);
	close(device_fd);

	return 0;
}

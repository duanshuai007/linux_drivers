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

	/*
		1)SOL_SOCKET:通用套接字选项.
		2)IPPROTO_IP:IP选项.
		3)IPPROTO_TCP:TCP选项.　
		
		SO_BROADCAST　　　　　　允许发送广播数据　　　　　　　　　　　　int
		SO_DEBUG　　　　　　　　允许调试　　　　　　　　　　　　　　　　int
		SO_DONTROUTE　　　　　　不查找路由　　　　　　　　　　　　　　　int
		SO_ERROR　　　　　　　　获得套接字错误　　　　　　　　　　　　　int
		SO_KEEPALIVE　　　　　　保持连接　　　　　　　　　　　　　　　　int
		SO_LINGER　　　　　　　 延迟关闭连接　　　　　　　　　　　　　　struct linger
		SO_OOBINLINE　　　　　　带外数据放入正常数据流　　　　　　　　　int
		SO_RCVBUF　　　　　　　 接收缓冲区大小　　　　　　　　　　　　　int
		SO_SNDBUF　　　　　　　 发送缓冲区大小　　　　　　　　　　　　　int
		SO_RCVLOWAT　　　　　　 接收缓冲区下限　　　　　　　　　　　　　int
		SO_SNDLOWAT　　　　　　 发送缓冲区下限　　　　　　　　　　　　　int
		SO_RCVTIMEO　　　　　　 接收超时　　　　　　　　　　　　　　　　struct timeval
		SO_SNDTIMEO　　　　　　 发送超时　　　　　　　　　　　　　　　　struct timeval
		SO_REUSERADDR　　　　　 允许重用本地地址和端口　　　　　　　　　int
		SO_TYPE　　　　　　　　 获得套接字类型　　　　　　　　　　　　　int
		SO_BSDCOMPAT　　　　　　与BSD系统兼容　　　　　　　　　　　　　 int
		
		成功执行时，返回0。失败返回-1，errno被设为以下的某个值  
		EBADF：sock不是有效的文件描述词
		EFAULT：optval指向的内存并非有效的进程空间
		EINVAL：在调用setsockopt()时，optlen无效
		ENOPROTOOPT：指定的协议层不能识别选项
		ENOTSOCK：sock描述的不是套接字
	*/
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) {
		perror("setsockopt");
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

static void accept_and_add_new(void)
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

	/*如果flags为0,epoll_create1()和删除了过时size参数的epoll_create()相同*/
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
	
err:
	free(events);
	close(socket_fd);
	close(device_fd);

	return 0;
}

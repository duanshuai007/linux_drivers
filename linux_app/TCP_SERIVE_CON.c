#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_ID 5555

void func();

int main()
{
	int server_id,client_id;
	struct sockaddr_in server_addr,client_addr;
	pthread_t thread;
	int addr_len;
	printf("创建套接字...\n");
	server_id = socket(AF_INET,SOCK_STREAM,0);
	if(server_id < 0)
	{
		perror("socket");
		exit(1);
	}
	printf("套接字创建成功\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_ID);
	server_addr.sin_addr.s_addr = inet_addr("192.168.1.101");
	bzero(&(server_addr.sin_zero),0);
	if(bind(server_id,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}
	listen(server_id,10);
	addr_len = sizeof(struct sockaddr_in);
	while(1)
	{
		printf("等待用户链接\n");
		client_id = accept(server_id,(struct sockaddr *)&client_addr,&addr_len);
		if(client_id > 0)
		{
			pthread_create(&thread,NULL,func,(void *)&client_id);
		}
	}
}

void func(void *argv)
{
	int client_fd = *((int *)argv);
	char recvbuff[255];
	printf("进入子线程\n");
	memset(recvbuff,0,sizeof(recvbuff));
	if(recv(client_fd,recvbuff,sizeof(recvbuff),0) == -1)
	{
		perror("recv");
		exit(1);
	}
	printf("recvbuff is : %s\n",recvbuff);
	printf("退出子线程");
	close(client_fd);
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define PORT_ID 5555

int main()
{
	int sock_id,client_id;
	int sin_size;
	time_t now;
	int count = 0;
	pid_t pid;
	char recbuff[255];
	char sendbuff[255] = "i am duanshuai\n";
	struct sockaddr_in my_addr,client_addr;
	printf("创建套接字\n");
	sock_id = socket(AF_INET,SOCK_STREAM,0);
	if(sock_id < 0)
	{
		perror("socket");
		exit(1);
	}
	printf("套接字创建成功\n");
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT_ID);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero),8);
	if(bind(sock_id,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}
	printf("绑定端口%d",PORT_ID);
	if(listen(sock_id,5) == -1)
	{
		perror("listen");
		exit(1);
	}
	printf("监听端口\n");
	sin_size = sizeof(struct sockaddr_in);
	while(1)
	{
		if((client_id = accept(sock_id,(struct sockaddr *)&client_addr,&sin_size)) == -1)
		{
			perror("accept");
			exit(1);
		}
		pid = fork();
		if(pid == 0)
		{
			printf("进入子进程...\n");
			memset(recbuff,0,sizeof(recbuff));
			printf("服务器链接客户端%s\n",inet_ntoa(client_addr.sin_addr));
			if(recv(client_id,recbuff,sizeof(recbuff),0) == -1)
			{
				perror("rece");
				exit(1);
			}
			if(strcmp(recbuff,"GET_SYSTEM_TIME") == 0)
			{
				printf("查询系统时间\n");
				now = time(NULL);
				memset(sendbuff,0,sizeof(sendbuff));
				sprintf(sendbuff,"%24s\n",ctime(&now));
				send(client_id,sendbuff,sizeof(sendbuff),0);
			}
			printf("推出子进程...\n");
		}
		else
		{
			printf("PID = %d,count = %d\n",pid,count);
			if(pid < 0)
			{
				printf("创建进程失败\n");
				exit(1);
			}
			if(pid > 0)
			{
				printf("进入主进程...\n");
				wait(&pid);
				printf("count = %d\n",count);
				printf("关闭套接字\n");
				close(client_id);
			}
		}
	}
	close(sock_id);
	exit(0);
}

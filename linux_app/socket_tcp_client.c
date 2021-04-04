#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT_ID 5555

int main(int argc,char **argv)
{
	int sock_id;
	char sendbuff[255] = "GET_SYSTEM_TIME";
	char recvbuff[255];
	struct sockaddr_in my_addr;
	if(argc < 2)
	{
		printf("参数错误\n");
		exit(1);
	}
	sock_id = socket(AF_INET,SOCK_STREAM,0);
	if(sock_id < 0)
	{
		perror("socket");
		exit(1);
	}
	printf("创建套接字成功\n");
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT_ID);
	my_addr.sin_addr.s_addr = inet_addr(argv[1]);
	bzero(&(my_addr.sin_zero),8);
		if(connect(sock_id,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1)
		{
			perror("connect");
			exit(1);
		}
		printf("链接服务器%s\n",inet_ntoa(my_addr.sin_addr));
		if(send(sock_id,sendbuff,sizeof(sendbuff),0) == -1)
		{
			perror("send");
			exit(1);
		}
		memset(recvbuff,0,sizeof(recvbuff));
		if(recv(sock_id,recvbuff,sizeof(recvbuff),0) == -1)
		{
			perror("recv");
			exit(1);
		}
		printf("recvbuff =:%s",recvbuff);
		sleep(1);
	close(sock_id);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT_ID 6666

int main(int argc,char **argv)
{
	int sock_id,sock_client_id;
	struct sockaddr_in target_addr;
	char sendbuff[255] = "whosyourdaddy\n";
	if(argc < 2)
	{
		printf("参数错误\n");
		exit(1);
	}
	sock_id = socket(AF_INET,SOCK_DGRAM,0);
	if(sock_id < 0)
	{
		perror("socket");
		exit(1);
	}
	printf("套接字创建成功\n");
	target_addr.sin_family = AF_INET;
	target_addr.sin_port = htons(PORT_ID);
	target_addr.sin_addr.s_addr = inet_addr(argv[1]);
	printf("链接到服务器：%s\n",inet_ntoa(target_addr.sin_addr));
	bzero(&(target_addr.sin_zero),8);
	while(1)
	{
		if(sendto(sock_id,sendbuff,sizeof(sendbuff),0,(struct sockaddr *)&target_addr,sizeof(struct sockaddr)) == -1)
		{
			perror("sendto");
			exit(1);
		}
		printf("sendto is : %s\n",sendbuff);
		sleep(1);
	}
	close(sock_id);
	return 0;
}

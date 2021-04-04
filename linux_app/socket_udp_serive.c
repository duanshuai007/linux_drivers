#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_ID 6666

int main()
{
	int sock_id;
	struct sockaddr_in my_addr,from_addr;
	int rec_len;
	char recbuff[255];
	sock_id = socket(AF_INET,SOCK_DGRAM,0);
	if(sock_id < 0)
	{
		perror("socket");
		exit(1);
	}
	printf("套接字创建成功\n");
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT_ID);
	my_addr.sin_addr.s_addr = inet_addr("192.168.1.12");
	bzero(&(my_addr.sin_zero),0);
	if(bind(sock_id,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}
	printf("地址绑定成功，绑定到%s\n",inet_ntoa(my_addr.sin_addr));
	memset(recbuff,sizeof(recbuff),0);
	printf("recbuff晴空\n");
	rec_len = sizeof(struct sockaddr);
	while(1)
	{
		if(recvfrom(sock_id,recbuff,sizeof(recbuff),0,(struct sockaddr *)&from_addr,&rec_len) == -1)
		{
			perror("recvfrom");
			exit(1);
		}
		printf("recbuff is : %s\n",recbuff);
		sleep(1);
	}
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main()
{
	struct addrinfo hints,*res = NULL;
	int rc;

	memset(&hints,0,sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	rc = getaddrinfo("localhost",NULL,&hints,&res);
	if(rc != 0)
	{
		perror("fetaddrinfo");
		exit(0);
	}
	else
	{

		if(res->ai_flags == AI_CANONNAME)
			printf("res->ai_flag = AI_CANNONAME\n");
		if(res->ai_family == 2)
			printf("res->ai_family = AF_INET\n");
		if(res->ai_family == 23)
			printf("res->ai_family = AF_INET6\n");
		if(res->ai_socktype == 1)
			printf("res->ai_socktype = SOCK_STREAM\n");
		if(res->ai_socktype == 2)
			printf("res->ai_socktype = SOCK_DGRAM\n");
		if(res->ai_protocol == IPPROTO_UDP)
			printf("res->ai_protocol = IPPROTO_UDP\n");
		if(res->ai_protocol == IPPROTO_TCP)
			printf("res->ai_protocol = IPPROTO_TCP\n");
		printf("host name is %s\n",res->ai_canonname);
	}
	exit(0);
}

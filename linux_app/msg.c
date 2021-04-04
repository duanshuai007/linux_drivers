#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define STRUCT(a) typedef struct\
{\
	long int my_msg_type;\
	char some_text[100];\
}msgbuf;\
msgbuf a

int main()
{
//	struct msgbuf
//	{
//		long int my_msg_type;
//		char some_text[100];
//	};
//	struct msgbuf msg_mybuff;
	STRUCT(msg_mybuff);
	char PATH[255];
	int msgid;
	pid_t pid;
	key_t key;
	char buffer[1024];
	//利用一个中间介质进行通信
	sprintf(PATH,"%s/etc/config.ini",(char*)getenv("HOME"));
	//char *msgpath = "/mnt/user/H";
	key = ftok(PATH,'a');
	msgid = msgget(key,0666|IPC_CREAT);
	if(msgid == -1)
	{
		printf("消息队列创建失败!\n");
		return 1;
	}
	if((pid = fork()) == 0)
	{
		sleep(1);
		printf("在子进程中...\n");
		printf("pid = %d\n",getpid());
		msgrcv(msgid,(void *)&msg_mybuff,1024,0,0);
		printf("接收到主进程消息队列消息:%s\n",msg_mybuff.some_text);
	}
	else
	{
		printf("在主进程中...\n");
		printf("请输入字符串到消息队列!\n");
		fgets(buffer,1024,stdin);
		msg_mybuff.my_msg_type = 1;
		strcpy(msg_mybuff.some_text,buffer);
		msgsnd(msgid,(void *)&msg_mybuff,100,0);
		memset(msg_mybuff.some_text,0,100);
		//msgrcv(msgid,(void *)&msg_mybuff,1024,0,0);
		printf("写入消息队列中的字符串是：%s\n",msg_mybuff.some_text);
		wait(&pid);
		printf("主进程结束\n");
	}
	return 0;
}

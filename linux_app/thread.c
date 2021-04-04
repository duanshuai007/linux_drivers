#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void *thread_func(void *arg)
{
	printf("Create New Pthread!\n");
	printf("My lucky number is %d\n",*(int *)arg);
	printf("Pteread run over\n");
	return NULL;
}

int main(void)
{
	int err;
	pthread_t new_thread;
	int main_arg;
	main_arg = 5;
	err = pthread_create(&new_thread,NULL,thread_func,&main_arg);
	sleep(1);
	if(err != 0)
	{
		perror("thread create failed!\n");
		exit(1);
	}
	printf("main finish!\n");
	exit(0);
}

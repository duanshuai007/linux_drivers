#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

typedef struct threadpool_work
{
	void *(*process)(void *arg);
	void *arg;
	struct threadpool_work *next;
}threadpool_work_t;

typedef struct threadpool
{
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_ready;
	threadpool_work_t *queue_head;
	int shutdown;
	pthread_t *threadid;
	int max_thread_num;
	int cur_queue_size;
}threadpool_t;

void *work_thread(void *arg);
void *task_handle(void *arg);

static threadpool_t *pool = NULL;

void threadpool_init(int max_thread_num)
{
	int  i = 0;
	pool = (threadpool_t *)malloc(sizeof(threadpool_t));
	pthread_mutex_init(&(pool->queue_lock),NULL);
	pthread_cond_init(&(pool->queue_ready),NULL);
	pool->queue_head = NULL;
	pool->max_thread_num = max_thread_num;
	pool->cur_queue_size = 0;
	pool->shutdown = 0;
	pool->threadid = (pthread_t *)malloc(max_thread_num*sizeof(pthread_t));
	for(i=0;i<max_thread_num;i++)
	{
		pthread_create(&(pool->threadid[i]),NULL,work_thread,NULL);
	}
}

int threadpool_work_add(void *(*process)(void *arg),void *arg)
{
	threadpool_work_t *newwork = (threadpool_work_t *)malloc(sizeof(threadpool_work_t));
	newwork->process = process;
	newwork->arg = arg;
	newwork->next = NULL;
	pthread_mutex_lock(&(pool->queue_lock));

	threadpool_work_t *member = pool->queue_head;
	if(member != NULL)
	{
		while(member -> next != NULL)
			member = member->next;
		member->next = newwork;
	}
	else
	{
		pool->queue_head = newwork;
	}
	assert(pool->queue_head!=NULL);
	pool->cur_queue_size++;
	pthread_mutex_unlock(&(pool->queue_lock));
	pthread_cond_signal(&(pool->queue_ready));
	return 0;
}

int threadpool_destroy()
{
	int i;
	pool->shutdown = 1;
	pthread_cond_broadcast(&(pool->queue_ready));
	for(i=0;i<pool->max_thread_num;i++)
	{
		pthread_join(pool->threadid[i],NULL);
	}
	free(pool->threadid);

	threadpool_work_t *head = NULL;
	while(pool -> queue_head!=NULL)
	{
		head = pool->queue_head;
		pool->queue_head = pool->queue_head->next;
		free(head);
	}
	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));
	free(pool);
	pool = NULL;
	return 0;
}

void *work_thread(void *arg)
{
	printf("启动序号为0x%x的工作线程!\n",pthread_self());
	while(1)
	{
		pthread_mutex_lock(&(pool->queue_lock));
		while(pool->cur_queue_size == 0 && !pool -> shutdown)
		{
			printf("序号为0x%x的工作线程正在等待分配任务\n",pthread_self());
			pthread_cond_wait(&(pool->queue_ready),&(pool->queue_lock));
		}
		if(pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->queue_lock));
			printf("序号为0x%x即将退出\n",pthread_self());
			pthread_exit(NULL);
		}
		printf("序号为0x%x的工作线程开始处理任务\n",pthread_self());
		pool->cur_queue_size--;
		threadpool_work_t * worker = pool->queue_head;
		pool->queue_head = worker->next;
		pthread_mutex_unlock(&(pool->queue_lock));
		(*(worker->process))(worker->arg);
		free(worker);
		worker = NULL;
	}
}

void * task_handle(void *arg)
{
	printf("序号为0x%x的工作线程正在处理任务%d\n",pthread_self(),*(int*)arg);
	sleep(1);
	return NULL;
}

int main(int argc,char **argv)
{
	int i;
	threadpool_init(2);
	int *task = (int *)malloc(sizeof(int)*5);
	for(i=0;i<5;i++)
	{
		task[i] = i;
		threadpool_work_add(task_handle,&task[i]);
	}
	sleep(10);
	threadpool_destroy();
	free(task);
	return 0;
}

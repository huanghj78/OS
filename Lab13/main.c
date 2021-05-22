#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <sys/wait.h>
#include "thread_pool_data.h"


threadpool_t *pool = NULL;

void* threadpool_thread( void *);//线程池中工作线程要做的事情。
void *adjust_thread(); //管理者线程要做的事情。
int threadpool_free(threadpool_t *); //销毁

int thread_enqueue(thread_info tmp,int *index){
    if(pool->thread_queue_size == pool->thread_num) return false;
    pool->thread_queue[pool->thread_queue_rear] = tmp;
    *index = pool->thread_queue_rear;
    pool->thread_queue_rear = (pool->thread_queue_rear + 1) % pool->thread_num;
    pool->thread_queue_size ++;
    return true;
}

int thread_dequeue(){
    if(pool->thread_queue_size == 0) return false;
    pool->thread_queue_size --;
    pool->thread_queue[pool->thread_queue_front].stat = needed;
    pool->thread_queue_front = (pool->thread_queue_front + 1) % pool->thread_num;
    return true;
}


//做一些初始化操作
threadpool_t *threadpool_create(int num, int task_num,int queue_capacity){
    int i;
    int index[num];
    for(i = 0;i < num;i++){
        index[i] = i;
    }

    pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (pool == NULL){
        printf("malloc threadpool fail");
        threadpool_free(pool); /* 前面代码调用失败时，释放poll存储空间 */
        return NULL;
    }



    pool->thread_num = num;
    pool->busy_thr_num = 0;           //忙状态的线程数初始值为0
    pool->thread_queue_size = num;
    pool->task_queue_size = 0;                  /* 任务队列实际元素个数初始值为0 */
    pool->task_queue_capacity = queue_capacity; //任务队列最大元素个数。
    pool->task_queue_front = 0;
    pool->task_queue_rear = 0;
    pool->count = 0;
    pool->task_num = task_num;
    pool->shutdown = false; /* 不关闭线程池 */

    /* 线程队列开辟空间 */
    pool->thread_queue = NULL;
    pool->thread_queue = (thread_info*)malloc(sizeof(thread_info) * num);
    if(pool->thread_queue == NULL){
        printf("malloc info fail\n");
        threadpool_free(pool); /* 前面代码调用失败时，释放poll存储空间 */
        return NULL;
    }


    /* 任务队列开辟空间 */
    pool->task_queue = NULL;
    pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_capacity); //每个元素都是一个结构体，结构体中有一个函数指针和一个void*的指针。
    if (pool->task_queue == NULL)
    {
        printf("malloc task_queue fail\n");
        threadpool_free(pool); /* 前面代码调用失败时，释放poll存储空间 */
        return NULL;
    }

    for(i = 0;i < num;i++){
        pool->thread_queue[i].stat = ready;
        pool->thread_queue[i].ptid = 0;
    }

    /* 启动 num 个 work thread */
    for (i = 0; i < num; i++)
    {
        pthread_create(&(pool->thread_queue[i].ptid), NULL, threadpool_thread, &index[i]); /*pool指向当前线程池*/
        printf("start thread 0x%x  number is %d\n", (unsigned int)pool->thread_queue[i].ptid,index[i]);           //16进程打印线程id
    }
    pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool); /* 启动管理者线程 */
    //pthread_detach(pool->adjust_tid);                                       //把管理线程设置为分离的，系统帮助回收资源。
    printf("管理线程开启\n");
    printf("****************************************************************************\n");

    sleep(1); //等待线程创建完成再回到主函数中。
    return pool;  
}

/* 线程池中各个工作线程 */
void *threadpool_thread(void *arg)
{
	int *tmp = (int*)arg;
    int index = *tmp;
    int num = index;
    int ret = false;
	threadpool_task_t task; //'
    func_arg args;  
    
 
	while(true)
	{
        // printf("???\n");
		/* Lock must be taken to wait on conditional variable */
		/*刚创建出线程，等待任务队列里 有任务，否则阻塞等待任务队列里有任务后再唤醒接收任务*/
 
		/*queue_size == 0 说明没有任务，调 wait 阻塞在条件变量上, 若有任务，跳过该while*/
        while(true){
            //printf("???\n");
            if(pool->thread_queue[index].stat == needed) break;
            if(pool->shutdown){
                //sleep(1);
                printf("thread %d is exiting\n", num);
                pthread_exit(NULL);     /* 线程自行结束 */
		    }
        }

        task.function=pool->task_queue[ pool->task_queue_front ].function;
	 	task.arg = pool->task_queue[ pool->task_queue_front ].arg;
        pool->task_queue_size--;
        pool->task_queue_front = (pool->task_queue_front +1)%pool->task_queue_capacity;//队头指针向后移动一位。
		

        /*执行任务*/ 
		printf("=== thread %d start working ===\n", num);
		pool->thread_queue[index].stat = busy;
		pool->busy_thr_num++;                              /*忙状态线程数+1*/
		//(*(task.function))(task.arg);                 /*执行回调函数任务，相当于process(arg)  */
        args.num = num;
        args.arg = task.arg;
		(task.function)(args);

        /*任务结束处理*/ 
		printf("=== thread %d end working ===\n\n", num);
		usleep(10000); 
		pool->busy_thr_num--;                 /*处理掉一个任务，忙状态数线程数-1*/

        pool->thread_queue[index].stat = ready;
        ret = false;
        while(ret == false){
            //printf("???\n");
            ret = thread_enqueue(pool->thread_queue[index],&index);
        }

	// 	while( (pool->queue_size ==0) && (!pool->shutdown) ) //线程池没有任务且不关闭线程池。
	// 	{
	// 		printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
	// 		pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));//线程阻塞在这个条件变量上
 
	// 		/*清除指定数目的空闲线程，如果要结束的线程个数大于0，结束线程*/
	// 		if( pool->wait_exit_thr_num > 0)  /* 要销毁的线程个数大于0 */
	// 		{
	// 			pool->wait_exit_thr_num--;
 
	// 			/*如果线程池里线程个数大于最小值时可以结束当前线程*/
	// 			if (pool->live_thr_num > pool->min_thr_num) {
	// 				printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
	// 				pool->live_thr_num--;
	// 				pthread_mutex_unlock(&(pool->lock));
	// 				pthread_exit(NULL);
	// 			}
	// 		}
	// 	}
 
	// 	/*如果指定了true，要关闭线程池里的每个线程，自行退出处理*/
	// 	if(pool->shutdown)
	// 	{
	// 		pthread_mutex_unlock ( &(pool->lock) );
	// 		printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
	// 		pthread_exit(NULL);     /* 线程自行结束 */
	// 	}
 
	// 	//从任务队列里获取任务，是一个出队操作
	// 	task.function=pool->task_queue[ pool->queue_front ].function;
	// 	task.arg = pool->task_queue[ pool->queue_front ].arg;
 
	// 	pool->queue_front = (pool->queue_front +1)%pool->queue_max_size;//队头指针向后移动一位。
	// 	pool->queue_size--;
 
	// 	/*任务队列中出了一个元素，还有位置 ，唤醒阻塞在这个条件变量上的线程，现在通知可以有新的任务添加进来*/
	// 	pthread_cond_broadcast(&(pool->queue_not_full)); //queue_not_full另一个条件变量。
 
	// 	/*任务取出后，立即将 线程池琐 释放*/
	// 	pthread_mutex_unlock(&(pool->lock));
 
	// 	/*执行任务*/ 
	// 	printf("thread 0x%x start working\n", (unsigned int)pthread_self());
	// 	pthread_mutex_lock(&(pool->thread_counter));         /*忙状态线程数变量琐*/
	// 	pool->busy_thr_num++;                              /*忙状态线程数+1*/
	// 	pthread_mutex_unlock(&(pool->thread_counter));
	// 	//(*(task.function))(task.arg);                 /*执行回调函数任务，相当于process(arg)  */
	// 		(task.function)(task.arg);
 
	// 	/*任务结束处理*/ 
	// 	printf("thread 0x%x end working\n\n", (unsigned int)pthread_self());
	// 	usleep(10000); 
 
	// 	pthread_mutex_lock(&(pool->thread_counter));
	// 	pool->busy_thr_num--;                 /*处理掉一个任务，忙状态数线程数-1*/
	// 	pthread_mutex_unlock(&(pool->thread_counter));
 
	 }
	 pthread_exit(NULL);
}

void* adjust_thread(){
    //printf("                          Adjust thread start!\n");
    int ret = false;
    while(true){
        if(pool->task_queue_size > 0){
            while(1){
                ret = thread_dequeue();
                if(ret == true)break;
                sleep(1);
                printf("There is no ready thread, waiting....\n");
            }
            sleep(1);// 同步！！
            // pool->task_queue_front = (pool->task_queue_front + 1) % pool->task_queue_capacity;
            // pool->task_queue_size--;
        }
        if(pool->count == pool->task_num  ){
            break;
        }
        //else pool->shutdown = true;
    }
    printf("管理线程结束\n");
    pthread_exit(NULL);
}

int threadpool_free(threadpool_t *pool)
{
	if (pool == NULL) {
		return -1;
	}
	if (pool->task_queue) {   //释放任务队列
		free(pool->task_queue);
	}
    if(pool->thread_queue){
        free(pool->thread_queue);
    }
	free(pool);
	pool = NULL;
 
	return 0;
}

/* 向任务队列中， 添加一个任务 */
int threadpool_add(void *(*function)(func_arg), void *arg)
{
 
	/* ==为真，队列已经满， 调wait阻塞 */
	while ((pool->task_queue_size == pool->task_queue_capacity)){
        //printf("cap = %d\n",pool->task_queue_capacity);
        printf("                                     The task queue is full, waiting...\n");
        sleep(1);
    }

 
	/* 清空 工作线程 调用的回调函数 的参数arg */
	// if (pool->task_queue[pool->task_queue_rear].arg != NULL)
	// {
	// 	free(pool->task_queue[pool->task_queue_rear].arg);
	// 	pool->task_queue[pool->task_queue_rear].arg = NULL;
 
	// }
 
	/*添加任务到任务队列里*/
	pool->task_queue[pool->task_queue_rear].function = function; //在队列的尾部添加元素
	pool->task_queue[pool->task_queue_rear].arg = arg;
	pool->task_queue_rear = (pool->task_queue_rear + 1) % pool->task_queue_capacity; /* 队尾指针移动, 模拟环形 */
	pool->task_queue_size++;

 
	return 0;
}


/* 线程池中的线程，模拟处理业务 */
void *process(func_arg args)
{
	//printf("thread %x working on task %d\n ",pthread_self(),*(int *)arg);

    int thread_num = args.num;
    int* arg = (int*)args.arg;
    printf("thread %d working on task %d\n",thread_num,*arg);
	printf("                                                                 ------ task %d is end ------\n",*(int *)arg);
    pool->count++;
	return NULL;
}


int main(){
    int thr_num, task_capacity,task_num,i;
    int num[1000];
    printf("输入线程池中工作线程数目：");
    scanf("%d",&thr_num);
    printf("输入线程池中任务队列容量：");
    scanf("%d",&task_capacity);
    printf("输入所需执行的任务数：");
    scanf("%d",&task_num); 
    // for (i = 0; i < task_num; i++) {
	// 	num[i]=i;
	// }
    pool = threadpool_create(thr_num,task_num,task_capacity);
    if(pool == NULL){
        printf("Create thread pool fail\n");
        return 0;
    }
	for (i = 0; i < task_num; i++) {
        num[i] = i;
		threadpool_add(process, (void*)&num[i]);     /* 向任务队列中添加任务 */
        printf("                                           add task %d\n",i);
	}
	pthread_join(pool->adjust_tid,NULL);
	printf("Clean up thread pool\n");
	pool->shutdown = true;
    sleep(1);
    return 0;
}

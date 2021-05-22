#define ready 0
#define needed 1
#define busy 2
#define true 1
#define false 0
#define task_number 15;

typedef struct{
    void* arg;
    int num;
}func_arg;

typedef struct{
	void *(*function) (func_arg);/* 函数指针，回调函数 */
	void *arg;/* 上面函数的参数 */
}threadpool_task_t;/* 各子线程任务结构体 */
 


typedef struct{
    int stat;     /*0 = 就绪，1 = 被需要，2 = 忙碌*/
    pthread_t ptid;
}thread_info;/* 线程池中每个线程的信息*/    

/* 描述线程池相关信息 */
typedef struct threadpool_t {
    thread_info* thread_queue;                                                   
	pthread_t adjust_tid;               /* 存管理线程tid */
	threadpool_task_t *task_queue;      /* 任务队列 */
 
	int thread_num;                    /* 线程池最小线程数 */
	int busy_thr_num;                   /* 忙状态线程个数 */

    int count;
    int task_num;
 
	int task_queue_front;                    /* task_queue队头下标 */
	int task_queue_rear;                     /* task_queue队尾下标 */
	int task_queue_size;                     /* task_queue队中实际任务数 */
	int task_queue_capacity;                 /* task_queue队列可容纳任务数上限 */

    int thread_queue_front;
    int thread_queue_rear;
    int thread_queue_size;
 
	int shutdown;                       /* 标志位，线程池使用状态，1 =  true, 2 = false */

}threadpool_t;

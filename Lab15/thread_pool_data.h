// 线程池中线程的三个状态
#define ready 0 /* 线程就绪状态，表示当前可以执行任务 */
#define needed                                                         \
  1 /* 线程被需要状态，表示当前有任务需要它执行， \
管理线程通过置线程的状态为needed状态来通知有任务需要其去执行。*/
#define busy 2 /* 线程忙碌状态，表示当前线程正在执行任务 */
#define true 1
#define false 0
#define task_number 15 /* 所需执行的任务数，用于测试 */

/* 此结构体作为被执行的任务函数的参数，其中arg为任务函数本身所需的参数，num为执行该任务的线程的编号，用于测试
 */
typedef struct {
  void *arg;
  int num;
} func_arg;

/* 此结构体为任务结构体，包含了执行所需执行任务的一个函数指针，以及该函数的参数指针
 */
typedef struct {
  void *(*function)(func_arg); /* 函数指针，回调函数 */
  void *arg;                   /* 上面函数的参数 */
} threadpool_task_t;

/* 此结构体包含了线程的信息，包括了线程状态和线程的ID */
typedef struct {
  int stat; /*0 = 就绪，1 = 被需要，2 = 忙碌*/
  pthread_t ptid;
} thread_info; /* 线程池中每个线程的信息*/

/* 此结构体描述了线程池相关信息 */
typedef struct threadpool_t {
  thread_info *thread_queue;     /* 线程就绪队列 */
  pthread_t adjust_tid;          /* 管理线程dtid */
  threadpool_task_t *task_queue; /* 任务等待队列 */

  int thread_num;   /* 线程池的线程数 */
  int busy_thr_num; /* 忙状态线程个数 */

  int count;    /* 记录当前已完成的任务数 */
  int task_num; /* 所需完成的总任务数 */

  int task_queue_front;    /* task_queue队头下标 */
  int task_queue_rear;     /* task_queue队尾下标 */
  int task_queue_size;     /* task_queue队中实际任务数 */
  int task_queue_capacity; /* task_queue队列可容纳任务数上限 */

  int thread_queue_front; /* thread_queue队头下标 */
  int thread_queue_rear;  /* thread_queue队尾下标 */
  int thread_queue_size;  /* thread_queue队中实际线程数 */

  int shutdown; /* 标志位，线程池使用状态，1 =  true, 2 = false */

} threadpool_t;
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "thread_pool_data.h"

threadpool_t *pool = NULL;

void *threadpool_thread(void *);  //线程池中工作线程要做的事情。
void *adjust_thread();            //管理者线程要做的事情。
void threadpool_free(threadpool_t *);  //销毁

/* 将tmp入队，同时更新对应线程的下标index */
int thread_enqueue(thread_info tmp, int *index) {
  /* 若队列已满，则无法入队 */
  if (pool->thread_queue_size == pool->thread_num) return false;

  pool->thread_queue[pool->thread_queue_rear] = tmp;
  *index = pool->thread_queue_rear; /* 更新线程所在队列下标 */
  pool->thread_queue_rear = (pool->thread_queue_rear + 1) % pool->thread_num;
  pool->thread_queue_size++;
  return true;
}

/* 实现的是将队首线程出队，并设置其状态为needed，以通知其开始工作 */
int thread_dequeue() {
  /* 若队列为空，则无法出队 */
  if (pool->thread_queue_size == 0) return false;
  pool->thread_queue_size--;
  /* 更新线程状态 */
  pool->thread_queue[pool->thread_queue_front].stat = needed;
  pool->thread_queue_front = (pool->thread_queue_front + 1) % pool->thread_num;
  return true;
}

/*创建线程池并初始化，
返回值为指向该线程池的指针，
参数num表示线程池中的线程数目（不包括管理线程）,
queue_capacity为任务队列的容量 */
threadpool_t *threadpool_create(int num, int task_num, int queue_capacity) {
  int i;
  int index[num]; /* 作为线程在初始队列中的下标，同时也作为其标号，
   注意到，线程在队列中的下标是会随着出队入队而改变的，但标号不会改变，
   由于线程是依次创建，依次入队的，因此在最开始下标和标号是一致的，
   后续两者没有必然联系 */

  /* 进行初始化，使用额外的数组index作为线程的传入参数
  而不使用创建时候的循环变量是为了避免条件竞争。*/
  for (i = 0; i < num; i++) {
    index[i] = i;
  }
  /* 申请线程池的内存空间 */
  pool = (threadpool_t *)malloc(sizeof(threadpool_t));
  if (pool == NULL) {
    printf("malloc threadpool fail");
    threadpool_free(pool); /* 用于释放线程池动态申请的内存空间的函数 */
    return NULL;
  }

  /* 线程池结构体中变量的初始化 */
  pool->thread_num = num; /* 设置线程池的线程数目 */
  pool->busy_thr_num = 0; /* 忙状态的线程数初始值为0 */
  pool->thread_queue_size =
      num; /* 由于初始化，故当前可用的线程数就是总的线程数 */
  pool->task_queue_size = 0; /* 任务队列实际元素个数初始值为0 */
  pool->task_queue_capacity = queue_capacity; /*任务队列最大元素个数 */
  pool->task_queue_front = 0;
  pool->task_queue_rear = 0;
  pool->count = 0;
  pool->task_num = task_num;
  pool->shutdown = false; /* 不关闭线程池 */
  pthread_mutex_init(&(pool->thread_lock),NULL); /* 初始化互斥锁 */
  pthread_mutex_init(&(pool->task_lock),NULL);
  pthread_mutex_init(&(pool->count_lock),NULL);


  /* 线程队列开辟空间 */
  pool->thread_queue = NULL;
  /* 指针均先置为NULL，以便后续若有申请失败的时候调用
  threadpool_free()函数能便于判断有哪些空间需要释放，
  若某个指针为NULL，那意味着没有分配成功，
  便不需要释放，从而避免了double free */
  pool->thread_queue = (thread_info *)malloc(sizeof(thread_info) * num);
  if (pool->thread_queue == NULL) {
    printf("malloc info fail\n");
    threadpool_free(pool);
    return NULL;
  }

  /* 任务队列开辟空间 */
  pool->task_queue = NULL;
  pool->task_queue = (threadpool_task_t *)malloc(
      sizeof(threadpool_task_t) *
      queue_capacity); /* 每个元素都是一个结构体，结构体中有一个函数指针和一个void*
                          的指针*/
  if (pool->task_queue == NULL) {
    printf("malloc task_queue fail\n");
    threadpool_free(pool); /* 前面代码调用失败时，释放poll存储空间 */
    return NULL;
  }

  /* 线程状态信息初始化 */
  for (i = 0; i < num; i++) {
    pool->thread_queue[i].stat = ready;
    pool->thread_queue[i].ptid = 0;
  }

  /* 启动工作线程 */
  for (i = 0; i < num; i++) {
    pthread_create(&(pool->thread_queue[i].ptid), NULL, threadpool_thread,
                   &index[i]); /* pool指向当前线程池 */
    /* 十六进程打印线程id及其编号 */
    printf("start thread 0x%x  number is %d\n",
           (unsigned int)pool->thread_queue[i].ptid, index[i]);
  }
  /* 启动管理线程 */
  pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);
  // pthread_detach(pool->adjust_tid);
  // //把管理线程设置为分离的，系统帮助回收资源。
  printf("管理线程开启\n");
  printf("***********************************************************\n");

  sleep(1);  //等待线程创建完成再回到主函数中。
  return pool;
}

/* 线程池中各个工作线程 */
void *threadpool_thread(void *arg) {
  /* 读取传入的参数 */
  int *tmp = (int *)arg;
  int index = *tmp;

  int num = index;        /* 线程标号 */
  int ret = false;        /* 存储函数返回值 */
  threadpool_task_t task; /* 线程将要执行的任务 */
  func_arg args;          /* 传入任务函数的参数 */

  while (true) {
    /* 循环阻塞，不断地查看自身的状态，
    若为needed状态则跳出循环，准备开始执行任务 */
    while (true) {
      if (pool->thread_queue[index].stat == needed) break;
      /* 若线程池将要关闭，则线程自行结束 */
      if (pool->shutdown) {
        printf("thread %d is exiting\n", num);
        pthread_exit(NULL);
      }
    }
    /* 将要被执行的任务从等待队列中出队 */
    pthread_mutex_lock(&(pool->thread_lock));
    /* 若队列中有任务 */
    if(pool->task_queue_size > 0){
      task.function = pool->task_queue[pool->task_queue_front].function;
      task.arg = pool->task_queue[pool->task_queue_front].arg;
      pool->task_queue_size--;
      pool->task_queue_front =
        (pool->task_queue_front + 1) %
        pool->task_queue_capacity;  //队头指针向后移动一位。
    }
    else{
      pthread_mutex_unlock(&(pool->thread_lock));
      pool->thread_queue[index].stat = ready;
      continue;
    }
    pthread_mutex_unlock(&(pool->thread_lock));

    /* 执行任务 */
    printf("=== thread %d start working ===\n", num);
    pool->thread_queue[index].stat = busy;
    pool->busy_thr_num++; /* 忙状态线程数+1 */
    /* 设置参数并调用任务函数 */
    args.num = num;
    args.arg = task.arg;
    (task.function)(args);

    /* 任务结束处理 */
    printf("=== thread %d end working ===\n", num);
    //usleep(10000);
    //sleep(1);
    pool->busy_thr_num--; /* 处理掉一个任务，忙状态数线程数-1 */

    /* 工作线程重新回到就绪队列，等待新任务 */
    ret = false;
    while (ret == false) {
      pthread_mutex_lock(&(pool->thread_lock));
      ret = thread_enqueue(pool->thread_queue[index], &index);
      pthread_mutex_unlock(&(pool->thread_lock));
    }
    pool->thread_queue[index].stat = ready;
  }
  pthread_exit(NULL);
}

void *adjust_thread() {
  int ret = false;
  while (true) {
    /* 若任务等待队列中有任务，则尝试从线程就绪队列中出队一个线程来执行任务 */
    if (pool->task_queue_size > 0) {
      while (1) {
        /* 当线程就绪队列中没有线程的时候则循环阻塞，直到有就绪线程 */
        pthread_mutex_lock(&(pool->thread_lock));
        ret = thread_dequeue();
        pthread_mutex_unlock(&(pool->thread_lock));
        if (ret == true) break;
        sleep(1);
        printf("There is no ready thread, waiting....\n");
      }
      //usleep(10000); /* 此处为了避免条件竞争 */
    }
    //printf("count = %d \n",pool->count);
    /* 若任务全部执行完毕，则可以结束管理线程 */
    if (pool->count == pool->task_num ) {
      break;
    }
  }
  printf("管理线程结束\n");
  pthread_exit(NULL);
}


void threadpool_free(threadpool_t *pool) {
  if (pool == NULL) {
    return;
  }

  pthread_mutex_destroy(&(pool->thread_lock));
  pthread_mutex_destroy(&(pool->task_lock));
  pthread_mutex_destroy(&(pool->count_lock));

  /* 释放任务等待队列 */
  if (pool->task_queue) {
    free(pool->task_queue);
    pool->task_queue = NULL;
  }
  /* 释放线程就绪队列 */
  if (pool->thread_queue) {
    free(pool->thread_queue);
    pool->thread_queue = NULL;
  }
  free(pool);
  pool = NULL;

}

/* 向任务队列中， 添加一个任务
  参数为一个执行任务函数的函数指针以及指向其参数的指针 */
int threadpool_add(void *(*function)(func_arg), void *arg) {
  /* 当任务队列不满时才可添加，否则循环阻塞 */
  while ((pool->task_queue_size == pool->task_queue_capacity)) {
    printf(
        "                                     The task queue is full, "
        "waiting...\n");
    sleep(1);
  }

  /* 添加任务到任务队列里 */
  pthread_mutex_lock(&(pool->task_lock));
  pool->task_queue[pool->task_queue_rear].function = function;
  pool->task_queue[pool->task_queue_rear].arg = arg;
  pool->task_queue_rear =
      (pool->task_queue_rear + 1) % pool->task_queue_capacity;
  pool->task_queue_size++;
   pthread_mutex_unlock(&(pool->task_lock));
  return 0;
}

/* 线程池中的线程，模拟处理业务 */
void *process(func_arg args) {
  int randomcount = rand() % 1000;
  int thread_num = args.num;  /* 读取所在线程编号 */
  int *arg = (int *)args.arg; /* 读取任务函数参数 */
  printf("thread %d working on task %d\n", thread_num, *arg);
  /* 随机模拟工作时长 */
  for(int i = 0;i < randomcount;i++);
  printf(
      "                                                                 ------ "
      "task %d is end ------\n",
      *(int *)arg);
  /* 对与任务完成数目的计算需要进行互斥以免产生条件竞争 */
  pthread_mutex_lock(&(pool->count_lock));
  pool->count++; /* 任务指向完毕，计数加一 */
  pthread_mutex_unlock(&(pool->count_lock));
}

int main() {
  int thr_num, task_capacity, task_num, i;
  /* 用一个额外的数组存放任务编号而不用循环变量，从而避免条件竞争 */
  int num[1000];
  printf("输入线程池中工作线程数目：");
  scanf("%d", &thr_num);
  printf("输入线程池中任务队列容量：");
  scanf("%d", &task_capacity);
  /* 为符合正常从1开始的思维，将task_capacity-1 */
  //task_capacity -= 1;
  printf("输入所需执行的任务数：");
  scanf("%d", &task_num);
  /* 创建线程池 */
  pool = threadpool_create(thr_num, task_num, task_capacity);
  if (pool == NULL) {
    printf("Create thread pool fail\n");
    return 0;
  }
  /* 添加任务 */
  for (i = 0; i < task_num; i++) {
    num[i] = i;
    threadpool_add(process, (void *)&num[i]); /* 向任务队列中添加任务 */
    printf("                                                  add task %d\n",
           i);
  }
  /* 等待管理线程的结束，因为管理线程的结束意味着任务都执行完毕 */
  pthread_join(pool->adjust_tid, NULL);
  printf("Clean up thread pool\n");
  pool->shutdown = true;
  printf("Waiting for all the threads end...\n");
  sleep(2); /* 等待线程池的关闭 */
  return 0;
}
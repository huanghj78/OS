# Lab Week 13 实验报告
## 设计思路
### 总体思路
* 此程序实现了一个线程池的应用及测试
#### 实现方面
* 有两个队列，一个是任务等待队列，一个是线程就绪队列，前者存储待执行的任务，后者存储当前线程池中可用的线程。
* 两个队列通过一个管理线程进行管理，当管理线程发现任务等待队列中有任务时，会去查看线程就绪队列中有无就绪线程.
  * 若有则会将任务等待队列和任务就绪队列的队首各自出队，然后将出队的任务交付给出队的线程执行，执行完毕之后线程会重新回到就绪队列中等待下一个任务的分配。
  * 若无则会先进行等待，直到有就绪队列中有就绪线程。
#### 测试方面
* 给定线程池的线程数以及所需执行的任务数，观察线程的调度情况和任务的完成情况。
### 具体思路
#### 数据结构方面
* 首先在一个.h文件中做了相关的结构体定义和宏定义，代码如下：
  ```c
  // 线程池中线程的三个状态
  #define ready 0 /* 线程就绪状态，表示当前可以执行任务 */
  #define needed                                                         \
    1 /* 线程被需要状态，表示当前有任务需要它执行， \
  管理线程通过置线程的状态为needed状态来通知有任务需要其去执行。*/
  # define busy 2 /* 线程忙碌状态，表示当前线程正在执行任务 */
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
    pthread_t adjust_tid;          /* 管理线程ptid */
    threadpool_task_t *task_queue; /* 任务等待队列 */
    pthread_mutex_t thread_lock;  /* 用于工作线程队列的互斥锁 */
    pthread_mutex_t task_lock;    /* 用于任务队列的互斥锁 */
    pthread_mutex_t count_lock;   /* 用于count的互斥锁 */

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

  ```
#### 函数方面
* 线程池创建函数
  threadpool_t *threadpool_create(int num, int queue_capacity)
  ```c
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
    printf("管理线程开启\n");
    printf("***********************************************************\n");

    sleep(1);  //等待线程创建完成再回到主函数中。
    return pool;
  }
  ```
* 线程入队函数
  int thread_enqueue(thread_info tmp,int *index)
  ```c
  /* 将tmp入队，同时更新对应线程的下标index */
  int thread_enqueue(thread_info tmp,int *index){
    /* 若队列已满，则无法入队 */
    if(pool->thread_queue_size == pool->thread_num) return false;
    
    pool->thread_queue[pool->thread_queue_rear] = tmp;
    *index = pool->thread_queue_rear; /* 更新线程所在队列下标 */
    pool->thread_queue_rear = (pool->thread_queue_rear + 1) % pool->thread_num;
    pool->thread_queue_size ++;
    return true;
  }
  ```
* 线程出队函数
  ```c
  /* 实现的是将队首线程出队，并设置其状态为needed，以通知其开始工作 */
  int thread_dequeue(){
    /* 若队列为空，则无法出队 */
    if(pool->thread_queue_size == 0) return false;
    pool->thread_queue_size --;
    /* 更新线程状态 */
    pool->thread_queue[pool->thread_queue_front].stat = needed;
    pool->thread_queue_front = (pool->thread_queue_front + 1) % pool->thread_num;
    return true;
  }
  ```
* 工作线程执行的函数
  void *threadpool_thread(void *arg)
  ```c
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
  ```
* 管理线程函数
  void* adjust_thread()
  ```c
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
  ```
* 线程池释放函数
  void threadpool_free(threadpool_t *pool)
  ```c
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
  ```
* 添加任务函数
  int threadpool_add(threadpool_t *pool, void *(*function)(func_arg), void *arg)
  ```c
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
  ```
* 模拟任务函数，用于测试
  ```c
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
  ```

## 设计过程
设计过程中遇到了一些问题，主要如下
* 同步线程所在队列下标
  * 由于各个工作线程的状态是存储在线程池中的队列中，因此各个线程需要直到自己在队列中的下标，才能获得自身的状态，而由于不断地出队入队，线程的队列下标会一直改变，因此需要对其进行同步。同步的方法是在线程中调用入队函数的时候，将下标变量的地址传入，在入队函数中获得入队之后新的下标进行返回。
* 一开始在未考虑互斥问题的时候，会遇到很多的问题，如最后结束输出的线程编号不正确或无法关闭线程池，因为关闭线程池是由计算已完成的任务数等于需完成的任务数时才会关闭，而当没有对count设置互斥时，可能有多个线程同时对count进行操作，导致计算错误，从而无法正常结束。
* 考虑互斥问题之后，引入了三个互斥锁，分别用于处理工作线程队列、任务队列以及已完成任务数count的互斥。
* 死锁：这是由于设计互斥锁的时候违背原则所引起
  ![3.png](https://pic.gksec.com/2021/05/18/42e4e2b35ce65/3.png)
  一开始在continue之前没有调用```pthread_mutex_unlock()```释放互斥锁，导致后面无法进入互斥锁中的内容。

## 程序测试
测试的程序主要在main函数中，代码如下
```c
int main(){
  int thr_num, task_capacity,task_num,i;
   /* 用一个额外的数组存放任务编号而不用循环变量，从而避免条件竞争 */
  int num[1000];
  printf("输入线程池中工作线程数目：");    
  scanf("%d",&thr_num);
  printf("输入线程池中任务队列容量：");
  scanf("%d",&task_capacity);
  printf("输入所需执行的任务数：");
  scanf("%d",&task_num); 
  /* 创建线程池 */
  pool = threadpool_create(thr_num,task_num,task_capacity);
  if(pool == NULL){
      printf("Create thread pool fail\n");
      return 0;
  }
  /* 添加任务 */
  for (i = 0; i < task_num; i++) {
    num[i] = i;
    threadpool_add(process, (void*)&num[i]);     /* 向任务队列中添加任务 */
    printf("                                                  add task %d\n",i);
  }
  /* 等待管理线程的结束，因为管理线程的结束意味着任务都执行完毕 */
  pthread_join(pool->adjust_tid,NULL);
  printf("Clean up thread pool\n");
  pool->shutdown = true;
  sleep(1);   /* 等待线程池的关闭 */
  return 0;
}
```
测试的方法只需运行主函数，依次输入线程池中的工作线程数，线程池的任务队列容量以及所需执行的任务总量即可。
### 测例1
![1.png](https://pic.gksec.com/2021/05/14/b4f961626d3ca/1.png)
* 首先，解释输出结果的框架：
  * 蓝色框内是线程池中的线程信息，包括ptid及编号
  * 下面的输出呈三列，左边一列为线程的工作情况，中间一列为任务添加情况，右边一列输出任务结束信号。
  * 黄色框是所有任务执行完毕之后的输出，会输出清空线程池，然后会输出各个线程的结束信号。
* 此测例的三个输入均为1，因此首先线程池中创建了一个工作线程，编号为0，然后开启管理线程。
* 接着添加了任务0到任务队列
* 管理线程监测到任务队列中有任务，便通知线程0开始工作，工作在任务0上。
* 工作一段时间，任务0结束，线程0结束工作
* 此时已没有其它任务，因此管理线程结束，然后关闭线程池，线程0也结束。
### 测例2
![2.png](https://pic.gksec.com/2021/05/14/928d9dd6f6005/2.png)
* 由于输入了2，故线程池中有两个线程，编号分别为0和1
* 任务队列容量输入了4，故一开始只能添加4个任务，然后就提示任务队列已满，正在等待
* 然后任务0分配给了线程0，分配之后任务队列不满，因此成功添加了任务4
* 任务1分配给了线程1，分配之后任务队列不满，因此成功添加了任务5
* 线程0完成任务0之后又依次转去执行任务2和任务4
* 线程1完成任务1之后又依次转去执行任务3和5
* 最后，所有任务执行完成之后，管理线程结束，工作线程也就随着结束，整个线程池清空。


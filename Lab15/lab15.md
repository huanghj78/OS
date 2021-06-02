# LabWeek15 实验报告
## 内容一
### alg.18-1-syn-fetch-1,2,3.c
#### 原理性解释
* 这几份代码是验证gcc的__sync_原子操作家族中的__sync_fetch_and_aad()和__sync_add_and_fetch()函数实现同步互斥的功能。
* type __sync_fetch_and_add (type *ptr, type value)：取ptr指向的变量的值，加上value之后再赋值给ptr指向的变量，整个过程是一个原子操作。
* type __sync_add_and_fetch (type *ptr, type value)：将ptr指向的变量加上value之后再去ptr指向的变量的值，同样的，整个过程是原子操作。
* 这两个函数类似于前自增和后自增，前者对应后自增，后者对应前自增。
#### 细节性解释
##### alg.18-1-syn-fetch-1.c
* 通过分别对```i = 10;```执行```__sync_fetch_and_add(&i, 20)```和``` __sync_add_and_fetch(&i, 20)```，输出函数返回值，同一语句中i的值和下一条语句i的值，探究并比较两个函数的作用及区别。
* 执行结果如下：
![1.png](https://pic.gksec.com/2021/05/27/98ccb4f595884/1.png)
可以看到，两个函数的返回值不相同，一个返回的是i初始的值，另一个返回的是i增加之后的值，这一点与自增类似，如下
![4.png](https://pic.gksec.com/2021/05/28/8bc25bb3cf2d1/4.png)
不同的是，当使用上述两个函数的时候，在同一个printf语句中，i的值不变，仍为10。
到了第二个printf语句，输出的i自然是加上20之后的值——30
##### alg.18-1-syn-fetch-2.c
* 通过创建40个线程，共同对count进行自增操作，使用__sync_fetch_and_add(&count, 1)实现互斥同步，即任一时刻最多只能有一个线程能够操作counter，从而保证了结果的正确性。
* 执行结果如下：
![2.png](https://pic.gksec.com/2021/05/28/e59e45245f750/2.png)
40个线程，分别令counter自增了2000次，因此最后counter为80000，结果正确。
##### alg.18-1-syn-fetch-3.c
* 在上一个的程序的基础上，将__sync_fetch_and_add(&count, 1)改为普通的自增，即counter++，此时无法实现互斥同步，因此结果出错。
* 执行结果如下：
![3.png](https://pic.gksec.com/2021/05/28/a4d6e88a56450/3.png)

### alg.18-2-syn-compare-test.c
#### 原理性解释
* 此程序是对以下四个函数功能的验证。
* bool __sync_bool_compare_and_swap (type* ptr, type oldval, type newval):比较* ptr与oldval的值，如果两者相等，则将newval更新到*ptr并返回true，否则返回false
* type __sync_val_compare_and_swap (type* ptr, type oldval type newval, ...)：比较* ptr与oldval的值，如果两者相等，则将newval更新到*ptr并返回操作**之前** *ptr的值，否则单纯返回操作**之前** *ptr
* type __sync_lock_test_and_set (type * ptr, type value)： 将value写入* ptr，对* ptr加锁，并返回操作之前*ptr的值
* void __sync_lock_release (type* ptr, ...)：将0写入到* ptr，并对*ptr解锁
#### 细节性解释
* 执行结果如下：
![5.png](https://pic.gksec.com/2021/05/28/3981b6e5367df/5.png)
* 首先调用__sync_bool_compare_and_swap()函数，要修改的值value为200000，指定alue原先的值oldvalue为123456，将要新赋给value的值newvalue为654321，由于value不等于oldvalue，故不会将newvalue赋值给value，因此返回值为为false，即0。value仍为200000
* 再次调用__sync_bool_compare_and_swap()函数，要修改的值value为200000，指定alue原先的值oldvalue为200000，将要新赋给value的值newvalue为654321，由于value等于oldvalue，故会将newvalue赋值给value，因此返回值为为true，即1。value变为654321
* 接着调用__sync_val_compare_and_swap()函数，要修改的值value为200000，指定alue原先的值oldvalue为123456，将要新赋给value的值newvalue为654321，由于value不等于oldvalue，故不会将newvalue赋值给value，因此返回值为200000。value仍为200000
* 再次调用__sync_bool_compare_and_swap()函数，要修改的值value为200000，指定alue原先的值oldvalue为200000，将要新赋给value的值newvalue为654321，由于value等于oldvalue，故会将newvalue赋值给value，因此返回值为200000。value变为654321
* 调用__sync_lock_test_and_set()函数，value的值为200000，将要新赋给value的值newvalue为654321，因此value的值被赋值为654321并返回了其原先的值200000，对value加锁
* 最后调用__sync_lock_release()函数，对value解锁并将其赋值为0

### alg.18-3-syn-pthread-mutex.c
#### 原理性解释
* 此程序是利用POSIX的互斥锁实现同步互斥，用到的主要函数如下：
* int pthread_mutex_init(pthread_mutex_t *restrict mutex,const pthread_mutexattr_t *restrict attr)：对互斥锁变量进行初始化，参数attr指定了初始化属性，若为NULL，则使用默认属性，此时等价于```pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;```。函数成功完成之后会返回零，其他任何返回值都表示出现了错误。
* int pthread_mutex_lock(pthread_mutex_t *mutex)：对互斥锁加锁。
* int pthread_mutex_unlock(pthread_mutex_t *mutex)：对互斥锁解锁，由于一个线程上锁之后，只有解锁了才能够继续上锁，因此互斥锁上锁之后到解锁之前的区域至多只能有一个线程可以访问，由此实现了互斥。
* int pthread_mutex_trylock(pthread_mutex_t *mutex)：销毁互斥锁
#### 细节性解释
* 定义了两个线程函数，功能均为对count进行20000次自增操作，区别在于一个使用了互斥锁对count的自增操作加锁，而另一个没有
* 主函数根据输入的参数选择调用其子一个线程函数，每次会创建40个子线程，即有40个子线程并发对count进行操作。
* 执行结果如下：
![6.png](https://pic.gksec.com/2021/05/28/d7e18d12e6d8e/6.png)
* 可以看到，当使用了互斥锁之后，count的结果是正确的，若没有使用，则结果错误。

### alg.18-4-syn-pthread-sem-unnamed.c
#### 原理性解释
* 此程序使用了POSIX的无名信号量来实现同步互斥，主要用的函数如下
* int sem_init(sem_t *sem, int pshared, unsigned int value)：初始化由 sem 指向的信号对象，并给它一个初始的整数值 value。pshared 控制信号量的类型，值为 0 代表该信号量用于多线程间的同步，值如果大于 0 表示可以共享，用于多个相关进程间的同步。
* int sem_wait(sem_t *sem)：是一个阻塞的函数，测试所指定信号量的值，它的操作是原子的。若 sem value > 0，则该信号量值减去 1 并立即返回。若sem value = 0，则阻塞直到 sem value > 0，此时立即减去 1，然后返回。
*   int sem_post(sem_t *sem)：指定的信号量 sem 的值加 1，唤醒正在等待该信号量的任意线程
* int sem_destroy(sem_t *sem)：用于对用完的信号量的清理。
* 在sem_wait和sem_post之间的即为临界区域。
#### 细节性解释
* 与上一个程序类似，定义两个线程函数，一个用信号量实现count++的同步，另一个单纯使用count++
* 主函数中对信号量进行初始化，并根据传入的参数选择调用相应的线程函数
* 执行结果如下：
![7.png](https://pic.gksec.com/2021/05/28/160f851100d20/7.png)
* 与使用互斥锁效果一样，当使用了信号量之后count结果正确，否则结果错误。
* 实际上，由于信号量初始化的值为1，此时其与互斥锁的功能是一样的，当将其初始化的值改为大于1，如改为100，如下：
![8.png](https://pic.gksec.com/2021/05/28/8b9c43f911f1f/8.png)
![9.png](https://pic.gksec.com/2021/05/28/9b924bfe8379e/9.png)
* 可以看到，即使使用了信号量，结果仍为错误的，可见，要注意信号量的初始化值，当其为1时，作用与互斥锁类似。

### alg.18-5-syn-pthread-sem-named.c
#### 原理性解释
* 此程序使用了POSIX的命名信号量来实现同步互斥，命名信号量即信号量有一个名称，不同进程可以通过该名字得到对应的信号量并进行操作。因此无名信号量和命名信号量的主要区别在于前者用于线程间的同步互斥而后者用于进程间的同步互斥。无名信号量直接保存在内存中，而命名信号量要求创建一个文件。
* 主要用的函数如下：
* sem_t *sem_open(const char *name, int oflag)或 sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value)，前者用于信号量存在时，打开该信号量。后者用于信号量不存在时，创建一个信号量。
    * name表示信号量文件名；
    * flags表示sem_open() 函数的行为标志；
    * mode表示文件权限；
    * value表示信号量初始值
    * 返回值：成功：信号量的地址；失败：SEM_FAILED
* int sem_close(sem_t *sem)：关闭sem指向的信号量。成功返回0，失败返回-1
* int sem_unlink(const char *name)：删除有名信号量的文件。成功返回0，失败返回-1
*  int sem_wait(sem_t *sem)：与无名信号量一样
* int sem_post(sem_t *sem)：与无名信号量一样
#### 细节性解释
* 此程序中，全局变量是一个指向信号量变量的指针named_sem，存放sem_open()函数的返回值，指向了一个线程之间可以共同感知的空间，因此调用相关函数的时候不需要再取地址。
* 两个线程函数的实现与上一个程序一样，一个使用了命名信号量，一个没有使用。
* 主函数中除了对命名信号量的初始化与无名信号量不同以外，其他与上一个程序一样。命名信号量是使用sem_open()进行初始化，并在最后以此调用sem_close()和sem_unlink()关闭信号量并删除对应的文件。
* 执行结果如下：
![10.png](https://pic.gksec.com/2021/05/28/9864d98b6bbb2/10.png)
* 同样的，当使用了信号量之后count结果正确，否则结果错误。
* 实际上，该程序将named_sem作为了全局变量，似乎体现不出命名信号量的优势所在，可改为如下图所示，结果仍然正确
![11.png](https://pic.gksec.com/2021/05/28/5a57bbe80c49e/11.png)
![12.png](https://pic.gksec.com/2021/05/28/b0efdb4a9b982/12.png)

### alg.18-6-syn-pc-con-6，7，8
#### 原理性解释
* 这几个程序实现的是多线程的生产者消费者问题，通过使用信号量来实现同步互斥问题。

#### 细节性解释
##### alg.18-6-syn-pc-con-6.h
* 定义了共享内存的结构体变量，共享内存中存放了控制用的数据（结构体ctln_pc_st）和问题相关的数据（data_pc_st），通过一个偏移量BASE_ADDR将两者隔开，即共享内存中前面一部分存放了用于控制的相关数据，后面是一个存放生产者生产的产品的循环队列。故要访问循环队列则需要加上偏移量BASE_ADDR
##### alg.18-6-syn-pc-con-6.c
* 此为控制台程序，主要功能是创建并初始化共享内存，创建生产者和消费者进程。
* 首先是先由用户输入确定共享内存中产品缓存区大小buffer_size，所需要生产的产品数max_item_num，生产者数目thread_pro和消费者数目thread_cons
* 然后由传入主函数的文件名创建共享内存，接着对共享内存中的数据以及信号量进行初始化。
    * 信号量stock用于缓冲区被占用的数目的同步
    * 信号量sem_mutex用于共享内存的互斥
    * 信号量emptyslot用于缓冲区中的空位置数目的同步
* 然后利用vfork()和execv()的组合生成两个进程分别去创建生产者和消费者线程。
* 最后等到两个子进程结束，即意味着生产消费完成，便销毁信号量并删除共享内存。
##### alg.18-7-syn-pc-producer-6.c
*  此程序为生产者程序，主函数实现的是根据传入的参数得到共享内存的ID，从而通过shmat()能够访问共享内存。接着通过pthread_create()生成生产者线程。最后等到生产者线程均结束之后，调用了THREAD_CONS - 1次的sem_post(&ctln->stock)```防止出现当循环队列中没有产品且生产产品数以达到要求时，生产者线程不再进入共享内存而导致一些消费者线程卡在了 sem_wait(&ctln->stock); 无法正常结束的情况 。```。最后断开与共享内存的连接。
* 生产者线程函数实现的是往共享内存中的循环队列加入一个新的元素，即生产一个产品。当目前生产的产品数未达到所需时，便尝试进入共享内存，由于生产的前提是需要循环队列中有空位，因此需要先调用``` sem_wait(&ctln->emptyslot);```,同时，为了实现互斥进入共享内存，则需要用到``` sem_wait(&ctln->sem_mutex);```。
* 当成功进入之后，则再判断目前生产的产品数是否达到所需，若达到了则调用```sem_post(&ctln->emptyslot);```释放空位。若没有，则将item_num++表示生产了一个产品，然后移动循环队列的尾指针```ctln->enqueue = (ctln->enqueue + 1) % ctln->BUFFER_SIZE;```，然后通过偏移量访问产品数据，写入产品的编号以及所被生产的线程号。
* 若此时已经达到所需生产数目，则将END_FLAG置为1表示可以结束生产。
* 最后调用```sem_post(&ctln->stock);```表示循环队列中多了一个产品，再调用``` sem_post(&ctln->sem_mutex);```以便其他线程可以进入共享内存。
##### alg.18-8-syn-pc-consumer-6.c
* 此程序为消费者程序，主函数实现的是根据传入的参数得到共享内存的ID，从而通过shmat()能够访问共享内存。接着通过pthread_create()生成消费者线程。最后等到消费者线程均结束之后，断开与共享内存的连接。
* 消费者线程函数实现的是取出共享内存中的循环队列中的一个产品。
* 当目前已消费的产品数目小于总数目或END_FLAG未被置1时，尝试进入共享内存。首先需要先确保循环队列中有产品，故需要调用```sem_wait(&ctln->stock); ```然后为了互斥地进入共享内存，再调用```sem_wait(&ctln->sem_mutex);```。
* 进入共享内存之后，再次判断目前已消费的产品数目是否小于总数目，若不是则调用```sem_post(&ctln->stock);```，否则移动队首指针表示出队，然后输出该产品的编号以及生产它的线程号，然后调用```sem_post(&ctln->emptyslot);```表明多了一个空位以便生产者线程可以进入。
* 最后调用```sem_post(&ctln->sem_mutex);```以便其他线程可以进入共享内存。
##### 运行结果
![13.png](https://pic.gksec.com/2021/05/29/63f61be4663fc/13.png)
* 输入了循环队列大小为4，所需生产产品数目为8，生产者数目为2，消费者数目为3
* 可以看到，首先输出了控制台进程以及由其生成的生产者进程和消费者进程的进程号。
* 接着，线程号为7521和7522的两个生产者线程分别生产了产品1和产2，此时缓冲区中有2个产品。
* 接着，线程号为7523和7524的两个消费者线程分别从中取出了产品1和产品2，此时缓冲区产品数又变为0，enqueue指针和dequeue指针均等于2。
* 然后线程号为7521的生产者线程生产了产品3，紧接着便被线程号为7525的消费者线程取出。
* 后面的分析同理，直到生产产品数到达所需且均被取出，则整个程序结束。

### alg.18-9-pthread-cond-wait.c
#### 原理性解释
* 此程序是通过**条件变量**来实现两个线程对一个变量分别进行递增和递减的操作同步，保证该变量始终大于0。
* 与互斥锁不同，条件变量是用来等待而不是用来上锁的。条件变量用来自动阻塞一个线程，直到某特殊情况发生为止。条件变量使我们可以睡眠等待某种条件出现。
* 条件变量是利用线程间共享的全局变量进行同步的一种机制，
主要包括两个动作：
一个线程等待"条件变量的条件成立"而挂起；另一个线程使"条件成立"（给出条件成立信号）。
* 条件变量的类型为pthread_cond_t，需要与互斥锁搭配使用，相关的函数如下：
    * int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr)：使用 cond_attr 指定的属性初始化条件变量 cond，当 cond_attr 为 NULL 时，使用缺省的属性
    *  int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)：阻塞该线程直到条件成立
    * int pthread_cond_signal(pthread_cond_t *cond)：发出条件成立信号，唤醒阻塞的线程
    * int pthread_cond_destroy(pthread_cond_t *cond)：销毁一个条件变量，释放它拥有的资源。进入 pthread_cond_destroy 之前，必须没有在该条件变量上等待的线程。否则返回EBUSY
#### 细节性解释
* 首先定义并初始化互斥锁mutex和条件变量cond，主函数实现的是创建两个线程，分别递增和递减全局变量count
* 线程函数decrement()实现的是对count进行递减操作，通过一个条件变量，当count小于等于0时，会通过``` pthread_cond_wait(&cond, &mutex)```堵塞直到count大于0，才会继续执行递减操作。
* 线程函数increment()实现的是对count进行递增操作，当count大于0的时候会通过```pthread_cond_signal(&cond)```唤醒阻塞的递减线程。
* 运行结果如下：
![14.png](https://pic.gksec.com/2021/05/29/a203a60459e07/14.png)
* 可以看到通过条件变量的控制，保证了count始终大于0

## 内容二
### 修改思路：
* 总共用到了三个信号量，分别为：
    * empty_slots：其值范围为0到task_queue_capacity，代表任务队列空位置的数目，用于实现任务队列的同步
    * count_mutex：其值为0或1，用于实现count的互斥
    * thread_mutex：其值为0或1，用于实现线程队列的互斥
* empty_slots所用在的地方如下所示：
* 每次添加任务到任务队列之前，先调用```sem_wait(&(pool->empty_slots));```，当任务队列没有空位的时候便会阻塞于此。
![选区_021.png](https://pic.gksec.com/2021/05/29/3c5a7ed5a6352/选区_021.png)
* 每次任务结束的时候，会调用```sem_post(&(pool->empty_slots));```表示任务队列有新的空位。
![选区_022.png](https://pic.gksec.com/2021/05/29/ac95699f9feaa/选区_022.png)
由此实现任务进队出队的同步。

* count_mutex所用在的地方如下所示：
![选区_023.png](https://pic.gksec.com/2021/05/29/4929c3afc6b19/选区_023.png)
* 实际上类似于一个互斥锁的功能，将count的自增操作实现互斥，避免条件竞争，保证结果的正确性。

* thread_mutex所用在的地方如下所示：
* 为了避免多个线程枪同一个任务的情况，需要对工作线程获取任务的过程进行互斥。
![选区_024.png](https://pic.gksec.com/2021/05/29/568b25a0778fd/选区_024.png)
* 为保证工作线程下标的正确以及队列的正确，入队需要进行互斥。
![选区_025.png](https://pic.gksec.com/2021/05/29/080cc33bae2c4/选区_025.png)
* 同样的道理，出队也需要进行互斥。
![选区_026.png](https://pic.gksec.com/2021/05/29/3f78eeddfaed7/选区_026.png)

### 结果分析：
![选区_029.png](https://pic.gksec.com/2021/05/29/97bad80bf444f/选区_029.png)
* 由于输入了2，故线程池中有两个线程，编号分别为0和1
* 任务队列容量输入了4，故一开始只能添加4个任务，然后就提示任务队列已满，正在等待
* 然后任务0分配给了线程0，分配之后任务队列不满，因此成功添加了任务4
* 任务1分配给了线程1，分配之后任务队列不满，因此成功添加了任务5
* 线程0完成任务0之后又依次转去执行任务2和任务4
* 线程1完成任务1之后又依次转去执行任务3和5
* 最后，所有任务执行完成之后，管理线程结束，工作线程也就随着结束，整个线程池清空。

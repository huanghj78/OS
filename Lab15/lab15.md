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
* 
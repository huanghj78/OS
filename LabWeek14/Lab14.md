# Lab Week 14
## 原理性解释
* 此实验是要求在已有的单个写线程和单个读线程的共享内存问题的基础之上，扩展为多个写线程和多个读线程，涉及多个线程访问同一个资源，那么就需要解决同步互斥的问题，倘若没有解决，那么可能会出现多个写线程在共享内存中写入信息，这样便会导致数据的冲突，导致内容出错；或是出现写线程和读线程同时在共享内存中的情况，此时也会导致读线程读出的信息可能是有误的，因此我们需要利用同步互斥，使上述这些情况不会发生。
* 预期实现的效果是：比如输入规定了有n个写线程，m个读线程，那么这n个写线程会互斥地进入共享内存中并写入信息，且只有当m个读线程都读到了信息之后下一个写线程才会继续往其中写入新的内容。
* 此实验主要用到了N个线程的peterson算法，便是实现并发程序之间互斥的软件实现。其中主要有两个变量——数组level和数组waiting。数组level表示每个线程的等待级别，最小为0，最高为N-1，-1表示未设置。数组waiting模拟了一个阻塞（忙等待）的线程队列，称之为waiting room，从位置0为入队列，位置越大则入队列的时间越长。每个线程为了进入临界区，需要在waiting room的每个位置都经过一次，如果没有更高优先级的线程或者被后入队列的线程踢出所在的waiting room，则当前线程会升级，在队列中向前走过一个位置。当走到最后一个waiting romm，即升到最高级，则可以进入临界区域，可见此算法可以实现互斥。
* 除此以外，还使用了POSIX的互斥锁，实际上单纯用peterson算法也可以实现，将peterson算法进行一个封装也就能得到一对互斥锁的操作。
* POSIX的互斥锁主要包含了以下函数：
    *  pthread_mutex_init(pmutex,NULL)：对互斥锁进行初始化
    *  int pthread_mutex_lock(pthread_mutex_t *mutex)：以原子操作方式给互斥锁加锁
    *   int pthread_mutex_unlock(pthread_mutex_t *mutex)：以原子操作方式给互斥锁解锁
    *  int pthread_mutex_destroy(pthread_mutex_t *mutex)：销毁互斥锁

## 设计思路
### 总体思路
* 首先，此实验要实现的是多个读线程和多个写线程对一块共享内存的同步访问。共享内存的实现用的是```shmget(),shmat()```等Linux的系统调用，多线程的实现用到的是POSIX的pthread，实现的同步主要用到了peterson算法和互斥锁。
* 对多个读线程和多个写线程的访问关系进行分析限定：
    * 当共享内存中已有读线程，则写线程不能进入，需等待读线程都退出之后才能进入，而其他的写线程仍可以进入。而在此基础上，对读线程的数量以及对同一内容的访问次数加上限定，从而防止了写线程遇到**饥饿**的情况。
    * 共享内存任何时刻最多只能有一个写线程在其中，且当有写线程在共享内存中时，读线程不能进入。
### 具体思路
* 首先，peterson算法用于实现共享内存中至多只有一个写线程，各个写线程之间通过一个peterson算法来实现互斥地进入共享内存。
* 实现读写线程之间的同步用的是一个标志位flag以及两个互斥锁flag_lock和read_cnt_lock
    * flag为1表示共享内存中有读线程或还有读线程未读取共享内存中的新内容，此时，经过peterson算法筛出来的写线程还不能进入共享内存，而是应该等到flag被置为0
    * 由于此程序会保证每个读线程都读取到共享内存中的最新信息之后新的写线程才会继续写入，因此需要一个read_cnt记录已经读取信息的线程数，为了避免多个线程修改read_cnt时产生条件竞争，需要一个互斥锁read_cnt_lock。当read_cnt == reader_num时，表明所有读线程都已读完，此时便将read_cnt恢复为0并将flag置为1，表明写线程可以进入共享内存了。为了避免多个线程修改flag时产生条件竞争，需要另外一个互斥锁flag_lock
* 代码如下：
    * 数据
    * 主函数：主函数实现的是共享内存和读写线程的创建
    ```c
    int main(int argc, char *argv[])
    {
        struct stat fileattr;
        int shmid; /* shared memory ID */
        void *shmptr;
        struct shared_struct *shared; /* structured shm */
        char pathname[80],  cmd_str[80];
        int shmsize, ret;

        pthread_t wptid[MAX];/*存储写线程的ptid */
        pthread_t rptid[MAX];/* 存储读线程的ptid */
        int wnum[MAX];/*存储写线程的编号 */
        int rnum[MAX];/* 存储读线程的编号 */

        /* 互斥锁初始化 */
        pthread_mutex_init(&read_cnt_lock,NULL);
        pthread_mutex_init(&flag_lock,NULL);

        /* 初始化共享内存大小 */
        shmsize = TEXT_NUM*sizeof(struct shared_struct);
        printf("max record number = %d, shm size = %d\n", TEXT_NUM, shmsize);

        if(argc <4) {
            printf("Usage: ./a.out pathname, number of writer, number of reader\n");
            return EXIT_FAILURE;
        }

        strcpy(pathname, argv[1]);
        sscanf(argv[2],"%d",&writer_num);/*读取写线程数*/
        sscanf(argv[3],"%d",&reader_num);/*读取读线程数*/

        if(stat(pathname, &fileattr) == -1) {
            ret = creat(pathname, O_RDWR);
            if (ret == -1) {
                ERR_EXIT("creat()");
            }
            printf("shared file object created\n");
        }
    
        key = ftok(pathname, 0x27); /* 0x27 a project ID 0x0001 - 0xffff, 8 least bits used */
        if(key == -1) {
            ERR_EXIT("shmcon: ftok()");
        }
        printf("key generated: IPC key = %x\n", key); /* can set any nonzero key without ftok()*/

        shmid = shmget((key_t)key, shmsize, 0666|PERM);
        if(shmid == -1) {
            ERR_EXIT("shmcon: shmget()");
        }
        printf("shmcon: shmid = %d\n", shmid);

        shmptr = shmat(shmid, 0, 0); /* returns the virtual base address mapping to the shared memory, *shmaddr=0 decided by kernel */

        if(shmptr == (void *)-1) {
            ERR_EXIT("shmcon: shmat()");
        }
        printf("shmcon: shared Memory attached at %p\n", shmptr);
        
        shared = (struct shared_struct *)shmptr;
        shared->flag = 0;/*初始化为0，即先让写线程写入信息，之后读线程才能读出信息*/

        sprintf(cmd_str, "ipcs -m | grep '%d'\n", shmid); 
        printf("\n------ Shared Memory Segments ------\n");
        system(cmd_str);
        
        for(int i = 0;i < writer_num;i++){
            wnum[i] = i;
        }
        /* 创建写线程 */
        for(int i = 0;i < writer_num;i++){
            ret = pthread_create(&wptid[i], NULL, &writer, (void *)&wnum[i]);
            if(ret != 0) {
                fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
            }
        }

        for(int i = 0;i < reader_num;i++){
            rnum[i] = i;
        }
        /* 创建读线程 */
        for(int i = 0;i < reader_num;i++){
            ret = pthread_create(&rptid[i], NULL, &reader, (void *)&rnum[i]);
            if(ret != 0) {
                fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
            }
        }

        /* 等待写线程和读线程的结束 */
        for (int i = 0; i < writer_num; i++) {
            ret = pthread_join(wptid[i], NULL);
            if(ret != 0) {
            perror("pthread_join()");
            }
        }
        for (int i = 0; i < reader_num; i++) {
            ret = pthread_join(rptid[i], NULL);
            if(ret != 0) {
            perror("pthread_join()");
            }
        }

        if(shmdt(shmptr) == -1) {
            ERR_EXIT("shmcon: shmdt()");
        }

        printf("\n------ Shared Memory Segments ------\n");
        system(cmd_str);
        /* 释放互斥锁 */
        pthread_mutex_destroy(&read_cnt_lock);
        pthread_mutex_destroy(&flag_lock);
        exit(EXIT_SUCCESS);
    }
    ```
    * 写线程函数：
    ```c
    void* writer(void* args){
        void *shmptr = NULL;
        struct shared_struct *shared = NULL;
        int shmid;
        int thread_num = *((int*)args);/* 读取传入的参数，作为线程号 */
        int lev,k,j;
        /* 获取共享内存的id */
        shmid = shmget((key_t)key, TEXT_NUM*sizeof(struct shared_struct), 0666|PERM);
        if (shmid == -1) {
            ERR_EXIT("shmwite: shmget()");
        }
        /* 利用id与共享内存建立连接 */
        shmptr = shmat(shmid, 0, 0);
        if(shmptr == (void *)-1) {
            ERR_EXIT("shmwrite: shmat()");
        }
        /* 获取共享内存指针 */
        shared = (struct shared_struct *)shmptr;
        
        for (lev = 0; lev < writer_num-1; ++lev) { /* there are at least writer_num-1 waiting rooms */
            level[thread_num] = lev;
            waiting[lev] = thread_num;
            while (waiting[lev] == thread_num) { /* busy waiting */
                /*  && (there exists k != thread_num, such that level[k] >= lev)) */
                for (k = 0; k < writer_num; k++) {
                    if(level[k] >= lev && k != thread_num) {
                        break;
                    }
                    if(waiting[lev] != thread_num) { /* check again */
                        break;
                    }
                } /* if any other proces j with level[j] < lev upgrades its level to or greater than lev during this period, then process thread_num must be kicked out the waiting room and waiting[lev] != thread_num, and then exits the while loop when scheduled */
                if(k == writer_num) { /* all other processes have level of less than process thread_num */
                    break;
                } 
            }
        } 

        /* critical section begin */
        while(shared->flag == 1){ /* 此时共享内存中有读线程，需要等待 */ 
            //sleep(1);
        }
        counter++;
        if (counter > 1) {
            printf("ERROR! more than one processes in their critical sections\n");
            kill(getpid(), SIGKILL);
        }
        //printf("I am the %d writer!\n",thread_num);
        /* 写入信息 */
        printf("I am the %d writer! Input the message:",thread_num);
        //scanf("%s",shared->mtext);
        fgets(shared->mtext,MAX,stdin);
        //sprintf(shared->mtext,"I am the %d writer!",thread_num);
        counter--;
        /* 将flag置为1表明已写入完毕，读线程可以开始读取内容 */
        pthread_mutex_lock(&flag_lock);
        shared->flag = 1;
        pthread_mutex_unlock(&flag_lock);
        /* critical section end*/

        /* allow other process of level max_num-2 to exit the while loop 
            and enter his critical section */
        level[thread_num] = -1;
        
        /* detach the shared memory */
        if(shmdt(shmptr) == -1) {
            ERR_EXIT("shmwrite: shmdt()");
        }

        pthread_exit(EXIT_SUCCESS);
    }
    ```
    * 读线程函数：
    ```c
    void* reader(void* args){
        void *shmptr = NULL;
        struct shared_struct *shared;
        int shmid;
        int thread_num = *((int*)args);/* 读取传入的参数，作为线程号 */
        
        /* 获取共享内存的id */
        shmid = shmget((key_t)key, TEXT_NUM*sizeof(struct shared_struct), 0666|PERM);
        if (shmid == -1) {
            ERR_EXIT("shread: shmget()");
        }
        /* 利用id与共享内存建立连接 */
        shmptr = shmat(shmid, 0, 0);
        if(shmptr == (void *)-1) {
            ERR_EXIT("shread: shmat()");
        }
        /* 获取共享内存指针 */
        shared = (struct shared_struct *)shmptr;
        
        /*每个读线程会读取writer_num次共享内存中的内容，
        即每次新的写线程写入信息之后，每个读线程都会从中读取内容*/
        for (int i = 0;i < writer_num;i++){
        pthread_mutex_lock(&read_cnt_lock);
        /* 所有读线程读取完毕*/ 
            if(read_cnt == reader_num){
                read_cnt = 0;
                pthread_mutex_lock(&flag_lock);
                shared->flag = 0;/*表明写线程可以进入共享内存*/
                pthread_mutex_unlock(&flag_lock);
            }
            read_cnt++;/*一个读线程进入共享内存之后read_cnt会加一*/
            pthread_mutex_unlock(&read_cnt_lock);
            
            while (shared->flag == 0) {
                sleep(1); /* 写进程在共享内存中，需等待 */
            }
            /* 读取信息 */
            printf("%*sI am reader %d, writer wrote: %s\n", 30, " ",thread_num, shared->mtext);
            sleep(2);/* 等待其他读线程读取完毕，否则该线程可能会重复读取 */
        }
        /* 断开与共享内存的连接 */
        if (shmdt(shmptr) == -1) {
            ERR_EXIT("shmread: shmdt()");
         }
    
        sleep(1);
        pthread_exit(NULL);
     }
    ```
## 结果分析
* 可以看到，执行此程序需要输入一个文件路径名，以用来创建共享内存，还需要输入写线程和读线程的数目。
![选区_001.png](https://pic.gksec.com/2021/05/25/5f4f248f46b29/选区_001.png)
* 当输入```./a.out /home 3 5```之后，即在/home目录下创建了共享内存，并指定了三个写线程和五个读线程。
* 首先是第0号写线程先进入了共享内存，故输出了```I am the 0 writer!```，然后即可在键盘输入，通过第0号写线程将信息写入到共享内存中，比如我输入了```hi!```，然后五个读线程则会并发地读出```hi!```。
* 接下来的第1号和第2号写线程分析同理。
* 可以看到各个读线程每次的顺序是不一样的，因为它们是并发的，进入共享内存中的顺序是随机的，实际上，由于写线程的同步互斥是使用peterson算法，因此写线程进入共享内存的顺序也是随机的，当写线程数目较多的时候就可看出来。
![选区_002.png](https://pic.gksec.com/2021/05/25/bf182942b77aa/选区_002.png)
* 下图是写线程数目为10的时候，此时就可以看到写线程不是按顺序的。
* ![选区_003.png](https://pic.gksec.com/2021/05/25/6b523e37a0bac/选区_003.png)

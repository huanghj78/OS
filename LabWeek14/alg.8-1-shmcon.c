#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <pthread.h>

#include "alg.8-0-shmdata.h"

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
        sleep(1);
    }
    counter++;
    if (counter > 1) {
        printf("ERROR! more than one processes in their critical sections\n");
        kill(getpid(), SIGKILL);
    }
    printf("I am the %d writer!\n",thread_num);
    /* 写入信息 */
    sprintf(shared->mtext,"I am the %d writer!",thread_num);
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


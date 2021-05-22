#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/shm.h>
#include <pthread.h>

#include "alg.8-0-shmdata.h"

void* reader(void* args)
{
    void *shmptr = NULL;
    struct shared_struct *shared;
    int shmid;

    int thread_num = *((int*)args);
 
    
    shmid = shmget((key_t)key, TEXT_NUM*sizeof(struct shared_struct), 0666|PERM);
    if (shmid == -1) {
        ERR_EXIT("shread: shmget()");
    }

    shmptr = shmat(shmid, 0, 0);
    if(shmptr == (void *)-1) {
        ERR_EXIT("shread: shmat()");
    }
    printf("%*sshmread: shmid = %d\n", 30, " ", shmid);    
    printf("%*sshmread: shared memory attached at %p\n", 30, " ", shmptr);
    printf("%*sshmread process ready ...\n", 30, " ");
    
    shared = (struct shared_struct *)shmptr;
    

    // while (1) {
    //     while (shared->written == 0) {
    //         sleep(1); /* message not ready, waiting ... */
    //     }

    //     printf("%*sI am reader %ld, writer wrote: %s\n", 30, " ",thread_num, shared->mtext);
    //     shared->written = 0;
    //     if (strncmp(shared->mtext, "end", 3) == 0) {
    //         break;
    //     }
    // } /* it is not reliable to use shared->written for process synchronization */
     
   for (int i = 0;i < writer_num;i++){
       pthread_mutex_lock(&read_cnt_lock);
        if(read_cnt == reader_num){
            reader_num = 0;
            shared->written = 0;
        }
        reader_num++;
        pthread_mutex_unlock(&read_cnt_lock);
        
        while (shared->written == 0) {
            sleep(1); /* message not ready, waiting ... */
        }

        printf("%*sI am reader %ld, writer wrote: %s\n", 30, " ",thread_num, shared->mtext);
   }

   if (shmdt(shmptr) == -1) {
        ERR_EXIT("shmread: shmdt()");
   }
 
    sleep(1);
    pthread_exit(NULL);
}

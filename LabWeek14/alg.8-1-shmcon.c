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
    //printf("%*sshmread: shmid = %d\n", 30, " ", shmid);    
    //printf("%*sshmread: shared memory attached at %p\n", 30, " ", shmptr);
    //printf("%*sshmread process ready ...\n", 30, " ");
    
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
            read_cnt = 0;
            pthread_mutex_lock(&written_lock);
            shared->written = 0;
            pthread_mutex_unlock(&written_lock);
        }
        read_cnt++;
        pthread_mutex_unlock(&read_cnt_lock);
        
        while (shared->written == 0) {
            sleep(1); /* message not ready, waiting ... */
        }

        printf("%*sI am reader %d, writer wrote: %s\n", 30, " ",thread_num, shared->mtext);
        sleep(2);
   }


   if (shmdt(shmptr) == -1) {
        ERR_EXIT("shmread: shmdt()");
   }
 
    sleep(1);
    pthread_exit(NULL);
}


 
void* writer(void* args)
{
    void *shmptr = NULL;
    struct shared_struct *shared = NULL;
    int shmid;
    int thread_num = *((int*)args);

    int lev,k,j;//

    char buffer[BUFSIZ + 1]; /* 8192bytes, saved from stdin */

    //printf("shmwrite: IPC key = %x\n", key);

    shmid = shmget((key_t)key, TEXT_NUM*sizeof(struct shared_struct), 0666|PERM);
    if (shmid == -1) {
        ERR_EXIT("shmwite: shmget()");
    }

    shmptr = shmat(shmid, 0, 0);
    if(shmptr == (void *)-1) {
        ERR_EXIT("shmwrite: shmat()");
    }
    //printf("shmwrite: shmid = %d\n", shmid);
    //printf("shmwrite: shared memory attached at %p\n", shmptr);
    //printf("shmwrite precess ready ...\n");
    
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

    /* critical section begin*/
    while(shared->written == 1){ sleep(1);};
    //shared->written = 0;
    counter++;
    if (counter > 1) {
        printf("ERROR! more than one processes in their critical sections\n");
        kill(getpid(), SIGKILL);
    }
    printf("I am the %d thread!\n",thread_num);
    sprintf(shared->mtext,"I am the %d thread!",thread_num);
    counter--;
    pthread_mutex_lock(&written_lock);
    shared->written = 1;
    pthread_mutex_unlock(&written_lock);
    /* critical section end*/

    level[thread_num] = -1;
    

    
    // while (1) {
    //     while (shared->written == 1) {
    //         sleep(1); /* message not read yet, waiting ... */ 
    //     }
 
    //     printf("Enter some text: ");
    //     fgets(buffer, BUFSIZ, stdin);
    //     strncpy(shared->mtext, buffer, TEXT_SIZE);
    //     printf("shared buffer: %s\n",shared->mtext);
    //     shared->written = 1;  /* message prepared */
 
    //     if(strncmp(buffer, "end", 3) == 0) {
    //         break;
    //     }
    // }
       /* detach the shared memory */
    if(shmdt(shmptr) == -1) {
        ERR_EXIT("shmwrite: shmdt()");
    }

//    sleep(1);
    pthread_exit(EXIT_SUCCESS);
}




int main(int argc, char *argv[])
{
    struct stat fileattr;
    int shmid; /* shared memory ID */
    void *shmptr;
    struct shared_struct *shared; /* structured shm */
    char pathname[80], key_str[10], cmd_str[80];
    int shmsize, ret;

    pthread_t wptid[MAX];
    pthread_t rptid[MAX];
    int wnum[MAX];
    int rnum[MAX];

    pthread_mutex_init(&read_cnt_lock,NULL);
    pthread_mutex_init(&written_lock,NULL);

    shmsize = TEXT_NUM*sizeof(struct shared_struct);
    printf("max record number = %d, shm size = %d\n", TEXT_NUM, shmsize);

    if(argc <4) {
        printf("Usage: ./a.out pathname, number of writer, number of reader\n");
        return EXIT_FAILURE;
    }
    strcpy(pathname, argv[1]);
    sscanf(argv[2],"%d",&writer_num);
    sscanf(argv[3],"%d",&reader_num);

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
    shared->written = 0;

    sprintf(cmd_str, "ipcs -m | grep '%d'\n", shmid); 
    printf("\n------ Shared Memory Segments ------\n");
    system(cmd_str);
	
    if(shmdt(shmptr) == -1) {
        ERR_EXIT("shmcon: shmdt()");
    }

    printf("\n------ Shared Memory Segments ------\n");
    system(cmd_str);

    sprintf(key_str, "%x", key);
    char *argv1[] = {" ", key_str, 0};

    for(int i = 0;i < writer_num;i++){
        wnum[i] = i;
    }

    for(int i = 0;i < writer_num;i++){
        ret = pthread_create(&wptid[i], NULL, &writer, (void *)&wnum[i]);
        if(ret != 0) {
            fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
        }
    }

    for(int i = 0;i < reader_num;i++){
        rnum[i] = i;
    }

    for(int i = 0;i < reader_num;i++){
        ret = pthread_create(&rptid[i], NULL, &reader, (void *)&rnum[i]);
        if(ret != 0) {
            fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
        }
    }

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







    // childpid1 = vfork();
    // if(childpid1 < 0) {
    //     ERR_EXIT("shmcon: 1st vfork()");
    // } 
    // else if(childpid1 == 0) {
    //     execv("./alg.8-2-shmread.o", argv1); /* call shm_read with IPC key */
    // }
    // else {
    //     childpid2 = vfork();
    //     if(childpid2 < 0) {
    //         ERR_EXIT("shmcon: 2nd vfork()");
    //     }
    //     else if (childpid2 == 0) {
    //         execv("./alg.8-3-shmwrite.o", argv1); /* call shmwrite with IPC key */
    //     }
    //     else {
    //         wait(&childpid1);
    //         wait(&childpid2);
    //              /* shmid can be removed by any process knewn the IPC key */
    //         if (shmctl(shmid, IPC_RMID, 0) == -1) {
    //             ERR_EXIT("shmcon: shmctl(IPC_RMID)");
    //         }
    //         else {
    //             printf("shmcon: shmid = %d removed \n", shmid);
    //             printf("\n------ Shared Memory Segments ------\n");
    //             system(cmd_str);
    //             printf("nothing found ...\n"); 
    //         }
    //     }
    // }
    pthread_mutex_destroy(&read_cnt_lock);
    pthread_mutex_destroy(&written_lock);
    exit(EXIT_SUCCESS);
}


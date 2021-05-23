#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/shm.h>
#include <signal.h>

#include "alg.8-0-shmdata.h"
 
void* writer(void* args)
{
    void *shmptr = NULL;
    struct shared_struct *shared = NULL;
    int shmid;
    int thread_num = *((int*)args);

    int lev,k,j;//

    char buffer[BUFSIZ + 1]; /* 8192bytes, saved from stdin */

    printf("shmwrite: IPC key = %x\n", key);

    shmid = shmget((key_t)key, TEXT_NUM*sizeof(struct shared_struct), 0666|PERM);
    if (shmid == -1) {
        ERR_EXIT("shmwite: shmget()");
    }

    shmptr = shmat(shmid, 0, 0);
    if(shmptr == (void *)-1) {
        ERR_EXIT("shmwrite: shmat()");
    }
    printf("shmwrite: shmid = %d\n", shmid);
    printf("shmwrite: shared memory attached at %p\n", shmptr);
    printf("shmwrite precess ready ...\n");
    
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
    while(shared->flag == 1);
    shared->flag = 0;
    counter++;
    if (counter > 1) {
        printf("ERROR! more than one processes in their critical sections\n");
        kill(getpid(), SIGKILL);
    }
    printf("I am the %ld thread!\n",thread_num);
    sprintf(shared->mtext,"I am the %ld thread!",thread_num);
    counter--;
    shared->flag = 1;
    /* critical section end*/
    

    
    // while (1) {
    //     while (shared->flag == 1) {
    //         sleep(1); /* message not read yet, waiting ... */ 
    //     }
 
    //     printf("Enter some text: ");
    //     fgets(buffer, BUFSIZ, stdin);
    //     strncpy(shared->mtext, buffer, TEXT_SIZE);
    //     printf("shared buffer: %s\n",shared->mtext);
    //     shared->flag = 1;  /* message prepared */
 
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

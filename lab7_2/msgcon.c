#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <mqueue.h>

#include "msgdata.h"

int main(int argc, char *argv[]){
    char pathname[80], cmd_str[80];
    mqd_t msqid, ret;
    pid_t childpid1, childpid2;
    struct msg_struct data;
    struct mq_attr mqAttr;
    if(argc < 2) {
        printf("Usage: ./a.out filename\n");
        return EXIT_FAILURE;
    }

    msqid = mq_open(argv[1], O_RDWR|O_CREAT,0666,NULL);
    if(msqid == -1) {
        ERR_EXIT("con: mq_open()");
    }   
    printf("con:msqid = %d\n",msqid);
    mq_getattr(msqid, &mqAttr);
    printf("con: current number of massages: %ld\n",mqAttr.mq_curmsgs);

    char *argv1[] = {" ", argv[1], 0};
    childpid1 = vfork();
    if(childpid1 < 0) {
        ERR_EXIT("msgcon: 1st vfork()");
    } 
    else if(childpid1 == 0) {
        execv("./msgsnd.o", argv1); /* call msgsender with filename */
    }
     else {
        childpid2 = vfork();
        if(childpid2 < 0) {
            ERR_EXIT("msgcon: 2nd vfork()");
        }
        else if (childpid2 == 0) {
            execv("./msgrcv.o", argv1); /* call msgreceiver with filename */
        }
        else {
            wait(&childpid1);
            wait(&childpid2);
            ret = mq_close(msqid);
            if(ret == -1){
                ERR_EXIT("con: mq_close()");
            }
            ret = mq_unlink(argv[1]); 
            if(ret == -1) {
                ERR_EXIT("con: mq_unlink()");
            }

        }
        exit(EXIT_SUCCESS);
    }


 

}
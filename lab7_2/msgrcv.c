#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mqueue.h>

#include "msgdata.h"

int main(int argc, char *argv[]){ /* Usage: ./b.out pathname msg_type */
    key_t key;
    struct stat fileattr;
    char pathname[80];
    int msqid, ret, count = 0;
    struct msg_struct data;
    struct mq_attr mqAttr;
    if(argc < 2) {
        printf("Usage: ./b.out pathname\n");
        return EXIT_FAILURE;
    }
    strcpy(pathname, argv[1]);
    if(stat(pathname, &fileattr) == -1) {
        ERR_EXIT("shared file object stat error");
    }

    msqid = mq_open(pathname, O_NONBLOCK|O_RDWR,0666,NULL);/* do not create a new msg queue */
    if(msqid == -1) {
        ERR_EXIT("msgget()");
    }

    mq_getattr(msqid, &mqAttr);
    while (1) {
        sleep(1);
        ret = mq_receive(msqid,data.mtext,mqAttr.mq_msgsize,&data.msg_type);
        if(ret == -1) { /* end of this msgtype */
            printf("\t\t\t\t\tnumber of received messages = %d\n", count);
            break;
        }
        printf("\t\t\t\t\treceive: %d %s\n", data.msg_type, data.mtext);
        count++;
        mq_getattr(msqid, &mqAttr);
        printf("\t\t\t\t\trev: current number of massages: %ld\n",mqAttr.mq_curmsgs);
    } 
    exit(EXIT_SUCCESS);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "alg.9-0-msgdata.h" 

int main(int argc, char *argv[]) /* Usage: ./b.out pathname msg_type */
{
    key_t key;
    struct stat fileattr;
    char pathname[80];
    int msqid, ret, count = 0;
    struct msg_struct data;
    long int msgtype = 0;   /* 0 - type of any messages */

    //argv[1]: pathname   
    //argv[2]: sleeping time 
    //argv[3]: msgtype
    if(argc < 3){
        printf("Usage: ./a.out pathname sleeping_time msg_type\n");
        return EXIT_FAILURE;
    }
    if(argc == 3)
        msgtype = 0;
    else {
        msgtype = atol(argv[3]);
        if (msgtype < 0)
            msgtype = 0;
    }    /* determin msgtype (class number) */

    strcpy(pathname, argv[1]);
    if(stat(pathname, &fileattr) == -1) {
        ERR_EXIT("shared file object stat error");
    }
    if((key = ftok(pathname, 0x27)) < 0) {
        ERR_EXIT("ftok()");
    }
    printf("\nIPC key = 0x%x\n", key);

    msqid = msgget((key_t)key, 0666); /* do not create a new msg queue */
    if(msqid == -1) {
        ERR_EXIT("msgget()");
    }

    int sleeping_time = atoi(argv[2]);

    while (1) {
        ret = msgrcv(msqid, (void *)&data, TEXT_SIZE, msgtype, IPC_NOWAIT); /* Non_blocking receive */
        if(ret == -1) { /* end of this msgtype */
            printf("number of received messages = %d\n", count);
            break;
        }
        printf("%ld %s\n", data.msg_type, data.mtext);
        count++;
        system("ipcs -q");
        sleep(sleeping_time);
    }
    
    struct msqid_ds msqattr;
    ret = msgctl(msqid, IPC_STAT, &msqattr);
    printf("number of messages remainding = %ld\n", msqattr.msg_qnum); 

    if(msqattr.msg_qnum == 0) {
        printf("do you want to delete this msg queue?(y/n)");
        if(getchar() == 'y') {
            if(msgctl(msqid, IPC_RMID, 0) == -1)
                perror("msgctl(IPC_RMID)");
        }
    }
   
    system("ipcs -q");
    exit(EXIT_SUCCESS);
}

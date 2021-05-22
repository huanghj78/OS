#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mqueue.h>

#include "msgdata.h"

int main(int argc, char *argv[]){
    char pathname[80];
    struct stat fileattr;
    long int msg_type;
    char buffer[TEXT_SIZE];
    int msqid, ret, count = 0;
    struct mq_attr mqAttr;
    FILE *fp;
    if(argc < 2) {
        printf("Usage: ./a.out pathname\n");
        return EXIT_FAILURE;
    }
    strcpy(pathname, argv[1]);

    if(stat(pathname, &fileattr) == -1) {
        ret = creat(pathname, O_RDWR);
        if (ret == -1) {
            ERR_EXIT("creat()");
        }
    }
    

    msqid = mq_open(pathname, O_NONBLOCK|O_RDWR,0666,NULL);
    if(msqid == -1) {
        ERR_EXIT("snd: mq_open()");
    }

    fp = fopen("./alg.9-0-msgsnd.txt", "rb");
    while (!feof(fp)) {
        ret = fscanf(fp, "%ld %s", &msg_type, buffer);
        if(ret == EOF) {
            break;
        }
        printf("%ld %s\n", msg_type, buffer);

        ret = mq_send(msqid, buffer, sizeof(buffer), msg_type); 
        if(ret == -1) {
            ERR_EXIT("mq_send()");
        }
        count++;
        sleep(1);
        mq_getattr(msqid, &mqAttr);
        printf("snd: current number of massages: %ld\n",mqAttr.mq_curmsgs);
    }
    printf("number of sent messages = %d\n", count);
    ret = mq_close(msqid);
    if(ret == -1){
        ERR_EXIT("msgsnd: mq_close()");
    }
    fclose(fp);
    exit(EXIT_SUCCESS);
}
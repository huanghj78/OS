#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <fcntl.h>

#include "shmdata.h"

int main(int argc, char *argv[])
{
    struct stat fileattr;
    key_t key; /* of type int */
    long long shmid; /* shared memory ID */
    void *shmptr;
    struct  shared_struct *shared; /* structured shm */
    pid_t childpid1, childpid2;
    char pathname[80], key_str[10], cmd_str[80];
    long long shmsize, ret;
    int shmctl_flag = 0;

    shmsize = sizeof(struct  shared_struct);

    if(argc <2) {
        printf("Usage: ./heep pathname\n");
        return EXIT_FAILURE;
    }
    strcpy(pathname, argv[1]);

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

    shmsize = 5091061760;
    while(1) {
        shmid = shmget((key_t)key, shmsize, 0666|PERM);
        if(shmid == -1) {
            printf("Fail to create %lld bytes !\n",shmsize);
            ERR_EXIT("shmget()");
        }
        printf("Create %lld bytes successfully!\n",shmsize);
        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            ERR_EXIT("shmcon: shmctl(IPC_RMID)");
        }
        shmsize ++;
    }
    exit(EXIT_SUCCESS);
}
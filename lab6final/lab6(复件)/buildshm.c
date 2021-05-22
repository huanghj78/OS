#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shmdata.h"
#define MAX 2147483647  // maximun of the ID

int main(int argc, char *argv[]) {
  struct stat fileattr;
  key_t key; /* of type int */
  int shmid; /* shared memory ID */
  void *shmptr;
  struct shared_struct *shared; /* structured shm */
  pid_t childpid1, childpid2;
  char pathname[80], key_str[10], cmd_str[80];
  int shmsize, ret;

  shmsize = sizeof(struct shared_struct);
  printf("max record number = %d, shm size = %d\n", TEXT_NUM, shmsize);

  if (argc < 2) {
    printf("Usage: ./a.out pathname\n");
    return EXIT_FAILURE;
  }
  strcpy(pathname, argv[1]);

  if (stat(pathname, &fileattr) == -1) {
    ret = creat(pathname, O_RDWR);
    if (ret == -1) {
      ERR_EXIT("creat()");
    }
    printf("shared file object created\n");
  }

  key = ftok(pathname,
             0x27); /* 0x27 a project ID 0x0001 - 0xffff, 8 least bits used */

  if (key == -1) {
    ERR_EXIT("shmcon: ftok()");
  }
  printf("key generated: IPC key = %x\n", key);
  shmid = shmget((key_t)key, sizeof(struct shared_struct), 0666 | PERM);
  if (shmid == -1) {
    ERR_EXIT("shmcon: shmget()");
  }
  printf("shmcon: shmid = %d\n", shmid);

  shmptr =
      shmat(shmid, 0, 0); /* returns the virtual base address mapping to the
                             shared memory, *shmaddr=0 decided by kernel */

  if (shmptr == (void *)-1) {
    ERR_EXIT("shmcon: shmat()");
  }
  printf("shmcon: shared Memory attached at %p\n", shmptr);

  shared = (struct shared_struct *)shmptr;

  //共享内存变量初始化
  int i;
  for (i = 1; i <= TEXT_NUM; i++) {
    shared->stu[i].id = MAX;
    shared->stu[i].flag = 1;
    shared->lock = 0;
  }

  if (shmdt(shmptr) == -1) {
    ERR_EXIT("shmcon: shmdt()");
  }

  exit(EXIT_SUCCESS);
}

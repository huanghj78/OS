#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alg.8-0-shmdata.h"

int main(int argc, char *argv[])
{
  void *shmptr = NULL;
  struct shared_struct *shared = NULL;
  int shmid;
  key_t key;

  char name[BUFSIZ + 1]; /* 8192bytes, saved from stdin */
  char id[BUFSIZ + 1];/* 8192bytes, saved from stdin */
  sscanf(argv[1], "%x", &key);

  printf("shmwrite: IPC key = %x\n", key);

  shmid = shmget((key_t)key, sizeof(struct shared_struct), 0666 | PERM);
  if (shmid == -1)
  {
    ERR_EXIT("shmwite: shmget()");
  }

  shmptr = shmat(shmid, 0, 0);
  if (shmptr == (void *)-1)
  {
    ERR_EXIT("shmwrite: shmat()");
  }
  printf("shmwrite: shmid = %d\n", shmid);
  printf("shmwrite: shared memory attached at %p\n", shmptr);
  printf("shmwrite precess ready ...\n");

  shared = (struct shared_struct *)shmptr;
  char flag[2];
  int tmp;
  while (1)
  {
    //结束进程
    if (shared->end == 1)
    {
      break;
    }
    while (shared->written == 1)
    {
      sleep(1); /* message not read yet, waiting ... */
    }
    //写入姓名
    printf("Enter the name: ");
    fgets(name, BUFSIZ, stdin);
    //若写入为＂ｅｎｄ＂则结束进程
    if(strncmp(name,"end",3) == 0){
      shared->written = 1;
      shared->end = 1;
      continue;
    }
    strcpy(shared->stu[shared->in].name, name);
    //写入学号
    printf("Enter the id: ");
    fgets(id, BUFSIZ, stdin);
    strcpy(shared->stu[shared->in].id, id);
    tmp = shared->in;
    printf("The name in the shared buffer: %s\n", shared->stu[shared->in].name);
    printf("The id in the shared buffer: %s\n", shared->stu[shared->in].id);
    shared->in = (shared->in + 1) % (TEXT_NUM + 1);
    //若输入的不为＂ｅｎｄ＂，则询问是否需要读出
    if (strncmp(shared->stu[tmp].name, "end", 3) != 0)
    {
      printf("Do you want to read the messages?(y/n)\n");
      fgets(flag, BUFSIZ, stdin);
    }
    //若输入的不为＂ｅｎｄ＂，则默认将ｆｌａｇ置为1，即输出队列中的信息
    else
    {
      strncpy(flag, "y", 1);
    }
    if ((shared->in + 1) % (TEXT_NUM + 1) == shared->out || strncmp(flag, "y", 1) == 0)
    {
      //若队列已满，则自动输出
      if ((shared->in + 1) % (TEXT_NUM + 1) == shared->out)
      {
        printf("The buffer is full\nThe reader will read\n");
      }
      shared->written = 1;
    }
    if (strncmp(shared->stu[shared->in].name, "end", 3) == 0)
    {
      shared->end = 1;
    }
  }
  /* detach the shared memory */
  if (shmdt(shmptr) == -1)
  {
    ERR_EXIT("shmwrite: shmdt()");
  }
  //    sleep(1);
  exit(EXIT_SUCCESS);
}

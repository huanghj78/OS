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
  struct shared_struct *shared;
  //int* test;
  int shmid;
  key_t key;

  sscanf(argv[1], "%x", &key);
  printf("%*sshmread: IPC key = %x\n", 30, " ", key);

  shmid = shmget((key_t)key, sizeof(struct shared_struct), 0666 | PERM);
  if (shmid == -1)
  {
    ERR_EXIT("shread: shmget()");
  }

  shmptr = shmat(shmid, 0, 0);
  if (shmptr == (void *)-1)
  {
    ERR_EXIT("shread: shmat()");
  }
  printf("%*sshmread: shmid = %d\n", 30, " ", shmid);
  printf("%*sshmread: shared memory attached at %p\n", 30, " ", shmptr);
  printf("%*sshmread process ready ...\n", 30, " ");

  shared = (struct shared_struct *)shmptr;
  int end = 0;
  while (1)
  {
    while (shared->written == 0)
    {
      if (shared->end == 1)
      {
        end = 1;
        break;
      }
      sleep(1); /* message not ready, waiting ... */
    }
    if (end == 1)
    {
      break;
    }
    if(shared->in != shared->out){
      //读出数据
      printf("%*sThe name you wrote: %s\n", 30, " ", shared->stu[shared->out].name);
      printf("%*sThe id you wrote: %s\n", 30, " ", shared->stu[shared->out].id);
      if (strncmp(shared->stu[shared->out].name, "end", 3) == 0)
      {
        break;
      }
      shared->out = (shared->out + 1) % (TEXT_NUM + 1);
    }
    //若队列已空，则堵塞
    if (shared->in == shared->out)
    {
      shared->written = 0;
    }
  } /* it is not reliable to use shared->written for process synchronization */

  if (shmdt(shmptr) == -1)
  {
    ERR_EXIT("shmread: shmdt()");
  }

  sleep(1);
  exit(EXIT_SUCCESS);
}

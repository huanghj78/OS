#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "shmdata.h"

struct shared_struct *shared;

void swap(int i, int j) {  //实现对堆指定的两个元素进行交换
  student tmp;
  tmp = shared->stu[i];
  shared->stu[i] = shared->stu[j];
  shared->stu[j] = tmp;
}

void percolate(int start, int end) {  //对堆从第start个元素到第end个元素进行下滤
  int tmp1, tmp2;
  int i = start;
  tmp1 = i;
  tmp2 = tmp1 << 1;
  while (tmp2 <= end) {
    if ((tmp2 | 1) <= end) {  //若右儿子不超过指定范围
      if ((shared->stu[tmp2].id < shared->stu[tmp1].id &&
           shared->stu[tmp2].flag == 0) ||
          (shared->stu[tmp2 | 1].id < shared->stu[tmp1].id &&
           shared->stu[tmp2 | 1].flag == 0)) {  //若存在一个儿子比该元素小
        if (shared->stu[tmp2].id <
            shared->stu[tmp2 | 1].id) {  //选择较小的儿子与该元素进行交换
          swap(tmp1, tmp2);
          tmp1 = tmp2;
          tmp2 = tmp1 << 1;
        } else {
          swap(tmp1, tmp2 | 1);
          tmp1 = tmp2 | 1;
          tmp2 = tmp1 << 1;
        }
      } else {
        break;
      }
    } else {  //若右儿子不超过指定范围，则只需要考虑左儿子
      if (shared->stu[tmp2].id < shared->stu[tmp1].id &&
          shared->stu[tmp2].flag == 0) {
        swap(tmp1, tmp2);
        tmp1 = tmp2;
        tmp2 = tmp1 << 1;
      } else {
        break;
      }
    }
  }
}

//将堆中的第index个节点向上过滤
void percolate1(int index) {
  int tmp1, tmp2;
  tmp1 = index;
  tmp2 = tmp1 >> 1;
  while (tmp2 >
         0) {  //从插入的位置开始，向堆顶进行一次向上过滤来维护插入后的小顶堆
    if (shared->stu[tmp1].id < shared->stu[tmp2].id &&
        shared->stu[tmp1].flag == 0 && shared->stu[tmp2].flag == 0) {
      swap(tmp1, tmp2);
    }
    tmp1 = tmp2;
    tmp2 = tmp1 >> 1;
  }
}

void pop(int n) {
  swap(1, n);
  percolate(1, n - 1);
}

void push() {  //向堆中输入一个元素
  char name[TEXT_SIZE];
  int id;
  student tmp;
  printf("name:");
  scanf("%s", name);
  printf("ID:");
  scanf("%d", &id);
  strcpy(tmp.name, name);
  tmp.id = id;
  tmp.flag = 0;
  int i = TEXT_NUM, tmp1, tmp2;
  while (shared->stu[i].flag == 0) {  //找到第一个空位置
    i--;
    if (i == 0) {
      printf("堆已满，无法再插入\n");
      return;
    }
  }
  shared->stu[i] = tmp;
  tmp1 = i;
  tmp2 = tmp1 >> 1;
  while (tmp2 >
         0) {  //从插入的位置开始，向堆顶进行一次向上过滤来维护插入后的小顶堆
    if (shared->stu[tmp1].id < shared->stu[tmp2].id) {
      swap(tmp1, tmp2);
    }
    tmp1 = tmp2;
    tmp2 = tmp1 >> 1;
  }
}

int search() {
  char name[TEXT_SIZE];
  int id;
  int i;
  printf("Name:");
  scanf("%s", name);
  printf("ID:");
  scanf("%d", &id);
  for (i = 1; i <= TEXT_NUM; i++) {
    if (strcmp(shared->stu[i].name, name) == 0 && shared->stu[i].id == id &&
        shared->stu[i].flag == 0) {
      return i;
    }
  }
  return -1;
}

void delete () {
  int i = TEXT_NUM;
  int index = search();
  if (index == -1) {
    printf("该元素不存在\n");
    return;
  }
  shared->stu[index].flag = 1;

  while (shared->stu[i].flag == 1) {
    i--;
  }
  swap(index, i);
  percolate1(index);
  percolate(index, i + 1);
  printf("该元素已被删除\n");
}

void modify() {
  char name[TEXT_SIZE];
  int id;
  int index;
  printf("要修改的学生姓名和学号\n");
  index = search();
  if (index == -1) {
    return;
  } else {
    printf("新的学生姓名和学号\n");
    printf("Name:");
    scanf("%s", name);
    printf("ID:");
    scanf("%d", &id);
    strcpy(shared->stu[index].name, name);
    shared->stu[index].id = id;
    printf("修改完成\n");
  }
}

void sort() {  //进行n次pop实现堆排序
  int i = 0;
  for (i = 0; i < TEXT_NUM; i++) {
    pop(TEXT_NUM - i);
  }
  printf("重排结果：\n");
  for (i = 1; i <= TEXT_NUM; i++) {
    if (shared->stu[i].flag == 0) {
      printf("Name:%s\n", shared->stu[i].name);
      printf("ID:%d\n\n", shared->stu[i].id);
    }
  }
}

int main() {
  void *shmptr = NULL;
  int temp;
  int shmid;
  key_t key;
  int option;  //选择变量
  int ex;      //例外标志位
  char name[TEXT_SIZE];
  int id;
  int out = 0;  //退出标志位
  student tmp;
  printf("Input the key:");
  scanf("%x", &key);
  //根据读取的key值打开对应的共享内存
  shmid = shmget((key_t)key, sizeof(struct shared_struct), 0666 | PERM);
  printf("THE SHMID IS %d\n", shmid);
  if (shmid == -1) {
    ERR_EXIT("shread: shmget()");
  }
  shmptr = shmat(shmid, 0, 0);
  if (shmptr == (void *)-1) {
    ERR_EXIT("shread: shmat()");
  }
  shared = (struct shared_struct *)shmptr;

  while (1) {
    ex = 0;
    int flag = 0;
    //判断是否可以进入共享内存
    while (shared->lock == 1) {
      if (flag == 0) {
        printf("等待进入共享内存空间...\n");
        flag = 1;
      }
      sleep(1); /* message not ready, waiting ... */
    }
    printf("已进入共享内存空间\n");
    shared->lock = 1;

    while (1) {
      printf(
          "请输入操作选项\n1 - 插入\n2 - 删除\n3 - 修改\n4 - 查找\n5 - 重排\n0 "
          "-退出\n");
      scanf("%d", &option);
      switch (option) {
        case 0:
          out = 1;
          break;
        case 1:
          push();
          break;
        case 2:
          delete ();
          break;
        case 3:
          modify();
          break;
        case 4:
          printf("NO. %d\n", search());
          break;
        case 5:
          sort();
          break;
        default:
          printf("违规输入，请重新输入\n");
          ex = 1;
          break;
      }
      if (ex == 1) {
        continue;
      }
      if (out == 1) {
        break;
      }
    }
    while (out == 1) {
      shared->lock = 0;
      printf(
          "输入-"
          "1删除共享内存并结束进程，输入0直接结束进程，输入1尝试再次进入内存空"
          "间\n");
      printf("等待输入...\n");
      scanf("%d", &temp);
      if (temp == 1 || temp == -1 || temp == 0) {
        out = 0;
        break;
      } else {
        printf("违规输入，请重新输入\n");
      }
    }
    if (temp == 1) {
      continue;
    }
    if (temp == -1 || temp == 0) {
      break;
    }
  } /* it is not reliable to use shared->written for process synchronization */

  if (shmdt(shmptr) == -1) {
    ERR_EXIT("shmread: shmdt()");
  }
  if (temp == -1) {
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
      ERR_EXIT("shmcon: shmctl(IPC_RMID)");
    }
  }
  sleep(1);
  exit(EXIT_SUCCESS);
}
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/shm.h>
#include "shmdata.h"
#define MIN 1
#define MAX 100

struct shared_struct *shared;

void swap(int i, int j)
{ //实现对堆指定的两个元素进行交换
    student tmp;
    tmp = shared->stu[i];
    shared->stu[i] = shared->stu[j];
    shared->stu[j] = tmp;
}

// void percolate(int start, int end)
// { //对堆从第start个元素到第end个元素进行下滤
//     int tmp1, tmp2;
//     int i = start;
//     tmp1 = i;
//     tmp2 = tmp1 << 1;
//     while (tmp2 <= end)
//     {
//         if ((tmp2 | 1) <= end)
//         { //若右儿子不超过指定范围
//             if (shared->stu[tmp2].id < shared->stu[tmp1].id || shared->stu[tmp2 | 1].id < shared->stu[tmp1].id)
//             { //若存在一个儿子比该元素小
//                 if (shared->stu[tmp2].id < shared->stu[tmp2 | 1].id)
//                 { //选择较小的儿子与该元素进行交换
//                     swap(tmp1, tmp2);
//                     tmp1 = tmp2;
//                     tmp2 = tmp1 << 1;
//                 }
//                 else
//                 {
//                     swap(tmp1, tmp2 | 1);
//                     tmp1 = tmp2 | 1;
//                     tmp2 = tmp1 << 1;
//                 }
//             }
//             else
//             {
//                 break;
//             }
//         }
//         else
//         { //若右儿子不超过指定范围，则只需要考虑左儿子
//             if (shared->stu[tmp2].id < shared->stu[tmp1].id)
//             {
//                 swap(tmp1, tmp2);
//                 tmp1 = tmp2;
//                 tmp2 = tmp1 << 1;
//             }
//             else
//             {
//                 break;
//             }
//         }
//     }
// }

void percolate(int start, int end)
{ //对堆从第start个元素到第end个元素进行下滤
    int tmp1, tmp2;
    int i = start;
    tmp1 = i;
    tmp2 = tmp1 << 1;
    while (tmp2 <= end)
    {
        if ((tmp2 | 1) <= end)
        { //若右儿子不超过指定范围
            if ((shared->stu[tmp2].id < shared->stu[tmp1].id && shared->stu[tmp2].flag == 0) || (shared->stu[tmp2 | 1].id < shared->stu[tmp1].id && shared->stu[tmp2 | 1].flag == 0))
            { //若存在一个儿子比该元素小
                if (shared->stu[tmp2].id < shared->stu[tmp2 | 1].id)
                { //选择较小的儿子与该元素进行交换
                    swap(tmp1, tmp2);
                    tmp1 = tmp2;
                    tmp2 = tmp1 << 1;
                }
                else
                {
                    swap(tmp1, tmp2 | 1);
                    tmp1 = tmp2 | 1;
                    tmp2 = tmp1 << 1;
                }
            }
            else
            {
                break;
            }
        }
        else
        { //若右儿子不超过指定范围，则只需要考虑左儿子
            if (shared->stu[tmp2].id < shared->stu[tmp1].id && shared->stu[tmp2].flag == 0)
            {
                swap(tmp1, tmp2);
                tmp1 = tmp2;
                tmp2 = tmp1 << 1;
            }
            else
            {
                break;
            }
        }
    }
}

void pop(int n)
{ //输出堆顶元素，同时把最后一个元素复制到堆顶并将最后一个元素置为-1，代表空，然后从1到n-1进行一次向下过滤
    swap(1, n);
    percolate(1, n - 1);
}

void push()
{ //向堆中输入一个元素
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
    while (shared->stu[i].flag == 0)
    { //找到第一个空位置
        i--;
        if (i == 0)
        {
            printf("堆已满，无法再插入\n");
            return;
        }
    }
    shared->stu[i] = tmp;
    tmp1 = i;
    tmp2 = tmp1 >> 1;
    while (tmp2 > 0)
    { //从插入的位置开始，向堆顶进行一次向上过滤来维护插入后的小顶堆
        if (shared->stu[tmp1].id < shared->stu[tmp2].id)
        {
            swap(tmp1, tmp2);
        }
        tmp1 = tmp2;
        tmp2 = tmp1 >> 1;
    }
}

void test()
{
    printf("查看第几个元素\n");
    int i;
    scanf("%d", &i);
    printf("name:%s\nID:%d\nflag:%d\n", shared->stu[i].name, shared->stu[i].id, shared->stu[i].flag);
}

int search()
{
    char name[TEXT_SIZE];
    int id;
    int i;
    printf("Name:");
    scanf("%s", name);
    printf("ID:");
    scanf("%d", &id);
    for (i = 1; i <= TEXT_NUM; i++)
    {
        if (strcmp(shared->stu[i].name, name) == 0 && shared->stu[i].id == id && shared->stu[i].flag == 0)
        {
            return i;
        }
    }
    return -1;
}

void delete ()
{
    int i = TEXT_NUM;
    int index = search();
    if (index == -1)
    {
        printf("该元素不存在\n");
        return;
    }
    shared->stu[index].flag = 1;

    while (shared->stu[i].flag == 0)
    {
        i--;
    }
    swap(index, i);
    percolate(index, i + 1);
    printf("该元素已被删除\n");
}

void sort()
{ //进行n次pop实现堆排序
    int i = 0;
    for (i = 0; i < TEXT_NUM; i++)
    {
        pop(TEXT_NUM - i);
    }
    printf("重排结果：\n");
    for (i = 1; i <= TEXT_NUM; i++)
    {
        if (shared->stu[i].flag == 0)
        {
            printf("Name:%s\n", shared->stu[i].name);
            printf("ID:%d\n\n", shared->stu[i].id);
        }
    }
}

void modify()
{
    char name[TEXT_SIZE];
    int id;
    int index;
    printf("要修改的学生姓名和学号\n");
    index = search();
    if (index == -1)
    {
        return;
    }
    else
    {
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
// int main(int argc, char *argv[])
int main()
{
    //int i, tmp;
    char buffer[20];
    void *shmptr = NULL;
    int temp;
    int shmid;
    key_t key;
    int flag = 0;
    int flag1 = 1;
    int option;
    int ex;
    char name[TEXT_SIZE];
    int id;
    int out = 0;
    student tmp;
    printf("Input the key:");
    scanf("%x", &key);
    shmid = shmget((key_t)key, sizeof(struct shared_struct), 0666 | PERM);
    printf("THE SHMID IS %d\n", shmid);
    if (shmid == -1)
    {
        ERR_EXIT("shread: shmget()");
    }
    shmptr = shmat(shmid, 0, 0);
    if (shmptr == (void *)-1)
    {
        ERR_EXIT("shread: shmat()");
    }
    shared = (struct shared_struct *)shmptr;

    while (1)
    {
        ex = 0;
        int fflag = 0;

        while (shared->lock == 1)
        {
            if (fflag == 0)
            {
                printf("等待进入共享内存空间...\n");
                fflag = 1;
            }
            sleep(1); /* message not ready, waiting ... */
        }

        if (flag == 0)
        {
            printf("已进入共享内存空间\n");
            flag1 = 0;
            shared->lock = 1;
            flag = 1;
        }

        while (1)
        {
            printf("请输入操作选项\n1 - 插入\n2 - 删除\n3 - 修改\n4 - 查找\n5 - 重排\n0 -退出\n");
            scanf("%d", &option);
            switch (option)
            {
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
            case 6:
                test();
                break;
            default:
                printf("违规输入，请重新输入\n");
                ex = 1;
                break;
            }
            if (ex == 1)
            {
                continue;
            }
            if (out == 1)
            {
                break;
            }
        }
        while (out == 1)
        {
            shared->lock = 0;
            printf("输入-1删除共享内存并结束进程，输入0直接结束进程，输入1尝试再次进入内存空间\n");
            printf("等待输入...\n");
            scanf("%d", &temp);
            if (temp == 1 || temp == -1 || temp == 0)
            {
                out = 0;
                break;
            }
            else
            {
                printf("违规输入，请重新输入\n");
            }
        }
        if (temp == 1)
        {
            continue;
        }
        if (temp == -1 || temp == 0)
        {
            break;
        }
    } /* it is not reliable to use shared->written for process synchronization */

    if (shmdt(shmptr) == -1)
    {
        ERR_EXIT("shmread: shmdt()");
    }
    if (temp == -1)
    {
        if (shmctl(shmid, IPC_RMID, 0) == -1)
        {
            ERR_EXIT("shmcon: shmctl(IPC_RMID)");
        }
    }
    sleep(1);
    exit(EXIT_SUCCESS);
}
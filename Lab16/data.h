#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#define RUNNING 1
#define READY 2
#define BLOCKED 3

typedef struct {
    struct timeval arrive_time;// 任务到达时间
    struct timeval start_time;// 任务开始时间
    struct timeval end_time;  // 任务结束时间
    int remaining_time;       // 任务剩余时间
    int wct;                /* 任务最坏情况执行时间，为了方便通过计算平均等待时间
                                来验证调度策略的正确性，尚且直接将其作为实际执行时间，
                                或者也可以通过一个随机数产生器来产生一个在wct范围内的
                                实际执行时间 */
    int priority;           // 任务的优先级
    int stat;       //任务状态，run = 1. ready = 2, blocked = 3
    sem_t sche;             // 任务的调度信号量
    pthread_t ptid;         // 任务所在线程的id
    struct task_info* next;     // 任务结点指针
}task_info;

pthread_t ptid[100];    // 存储任务线程的id
task_info* head;        // 就绪队列的头结点指针，指向头结点，头结点为不存储信息
task_info* tail;        // 就绪队列的尾指针，指向最后一个结点
int task_num = 0;       // 当前任务总数
int strategy;           // 所选调度策略，1-FCFS，2-SJB，3-优先级
int isCpuBusy = 0;      // CPU是否被占用的标志
int endFlag = 0;        // 输入结束标志
pthread_t cur_task_ptid;// 当前正在执行的任务的线程id
long long cur_shortest_time = 100;// 当前执行任务的最短时间
int cur_highest_priority = 100;// 当前执行任务的最高优先级
pthread_mutex_t output_lock;// 输出互斥锁，避免输出混乱




#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#define RUNNING 1
#define READY 2
#define BLOCKED 3

typedef struct {
    struct timeval arrive_time;
    struct timeval start_time;
    struct timeval end_time;
    int remaining_time;
    int wct;   //sec
    int priority;
    int stat;       //run = 1. ready = 2, blocked = 3
    sem_t sche;
    pthread_t ptid;
    struct task_info* next;
}task_info;

void sleep_ms(long int timegap_ms)
{
    struct timeval t;
/*  __time_t tv_sec;
    __suseconds_t tv_usec;
*/
    long curr_s, curr_ms, end_ms;

    gettimeofday(&t, 0);
    curr_s = t.tv_sec;
    curr_ms = (long)(t.tv_sec * 1000);
    end_ms = curr_ms + timegap_ms;
    while (1) {
        gettimeofday(&t, 0);
        curr_ms = (long)(t.tv_sec * 1000);
        if (curr_ms > end_ms) {
            break;
        }
    }
    return;
}



// void task_info_free(task_info* head){
//     task_info* p1 = head;
//     task_info* p2 = p1;
//     while(p1 != NULL){
//         _FEATURES_H
//     }
// }
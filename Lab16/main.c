#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>
#include <pthread.h>
#include "data.h"

pthread_t ptid[100];
task_info* head;
task_info* tail;
int task_num = 0;
int sel;
int isCpuBusy = 0;
int endFlag = 0;
pthread_t cur_task_ptid;
sem_t isCpuBusy_mutex;
pthread_mutex_t head_lock;
long long cur_remaining_time;
long long cur_shortest_time = 100;
int cur_highest_priority = 100;
pthread_mutex_t output_lock;

void insert(task_info* p){
    task_info* cur = head->next;
     task_info* tmp;
    int flag = 0;
    while(cur != NULL){
        flag = 1;
        if(cur == head->next && cur->priority > p->priority){
            head->next = p;
            p->next = cur;
        }
        if(cur->next == NULL && cur != head->next){
            cur->next = p;
            tail = p;
            break;
        }
        tmp = cur->next;
        if(cur->priority <= p->priority && tmp->priority > p->priority){
            p->next = cur->next;
            cur->next = p;
            break;
        }
        else{
            cur = cur->next;
        }
    }
    if(flag == 0){
        head->next = p;
        tail = p;
        p->next = NULL;
    }
}

void insert2(task_info* p){
    task_info* cur = head->next;
    task_info* tmp;
    int flag = 0;
    while(cur != NULL){
        flag = 1;
        if(cur == head->next && cur->remaining_time > p->remaining_time){
            head->next = p;
            p->next = cur;
        }
        if(cur->next == NULL && cur != head->next){
            cur->next = p;
            tail = p;
            break;
        }
        tmp = cur->next;
        if(cur->remaining_time <= p->remaining_time && tmp->remaining_time > p->remaining_time){
            p->next = cur->next;
            cur->next = p;
            break;
        }
        else{
            cur = cur->next;
        }
    }
    if(flag == 0){
        head->next = p;
        tail = p;
        p->next = NULL;
    }
}

void out(task_info* cur){
    task_info* tmp = head;
    while(tmp->next != cur){
        tmp = tmp->next;
    }
    tmp->next = cur->next;
    cur->next = NULL;
    tail = head;
    while(tail->next != NULL){
        tail = tail->next;
    }
}

void sighand(int signo)  {  
    pthread_t tid = pthread_self();  
    printf("Thread %lu in preempted\n", tid);  
    return;  
}

void *task_func(void* args){
    int* waiting_time = (int*)malloc(sizeof(int));
    task_info* task = (task_info*)args; 
    struct timeval task_start;
    struct timeval task_end;
    while(1){
        //printf("                          ptid(%ld) thread is created!\n",task->ptid);
        sem_wait(&(task->sche));
        gettimeofday(&task_start,0);
        isCpuBusy = 1;
        pthread_mutex_lock(&output_lock);
        printf("%*s%s%lld%s\n",100,"","线程号为",task->ptid,"开启");
        pthread_mutex_unlock(&output_lock);
        //printf("                           ptid(%ld) thread start!\n",task->ptid);
        task->stat = RUNNING;
        gettimeofday(&(task->start_time),0);
        task->remaining_time = sleep(task->remaining_time);
        pthread_mutex_lock(&output_lock);
        printf("remaining time = %d\n",task->remaining_time);
        pthread_mutex_unlock(&output_lock);
        if(task->remaining_time > 0){
            insert(task);
            continue;
        }
        gettimeofday(&task_end,0);
        //sleep_ms(task->wct);
        //sem_post(&(task->sche));
        gettimeofday(&(task->end_time),0); 
        printf("task->start_time.tv_sec = %ld\n",task->start_time.tv_sec);
        printf("task->arrive_time.tv_sec = %ld\n",task->arrive_time.tv_sec);
        *waiting_time = task->start_time.tv_sec - task->arrive_time.tv_sec + task_end.tv_sec - task_start.tv_sec - task->wct;
        // pthread_mutex_lock(&head_lock);
        // head->next = task->next;
        // pthread_mutex_unlock(&head_lock);
        free(task);
        pthread_mutex_lock(&output_lock);
        printf("%*s%s%ld%s\n",100,"","线程号为",task->ptid,"结束");
        pthread_mutex_unlock(&output_lock);
        isCpuBusy = 0;
        break;
    }

    
    pthread_exit((void*)waiting_time);
}

void* task_schedule(){
    printf("%*s%s%*s%s\n",55,"","任务调度情况",45,"","任务线程情况");
    //printf("                                                                任务调度情况\n");
    while(1){
        if(sel == 1){
            int semVal;
            int flag = 0;
            //task_info* cur = head->next;
            while(1){
                //sleep_ms(500);
                if(endFlag){
                    printf("%*s%s\n",55,"","结束");
                    break;
                }
                if(head->next == NULL){
                    //cur = head->next;
                    //printf("!!!!asdasdas");
                    if(flag == 0){
                        printf("%*s%s\n",55,"","当前无任务可调度...");
                    }
                    flag = 1;
                    continue;
                }
                else{
                    if(isCpuBusy){
                        continue;
                    }
                    else{
                        flag = 0;
                        task_info* cur = head->next;
                        sem_getvalue(&(cur->sche),&semVal);
                        if(semVal == 0 && cur->ptid != 0 && !isCpuBusy){
                            sem_post(&(cur->sche));
                            printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                            out(cur);
                            //pthread_join(cur->ptid,NULL);
                        } 
                    }
                }
            }
            long long* waiting_time;
            long long total_waiting_time = 0;
            for(int i = 0;i < task_num;i++){
                pthread_join(ptid[i],(void**)&waiting_time);
                total_waiting_time += (*waiting_time);
                
                free(waiting_time);
            }
            printf("平均等待时间为：%lld s\n",total_waiting_time / task_num);
            break;
        }
        if(sel == 2){
            int semVal;
            int flag = 0;
            //task_info* cur = head->next;
            while(1){
                if(endFlag){
                    pthread_mutex_lock(&output_lock);
                    printf("%*s%s\n",55,"","结束");
                    pthread_mutex_unlock(&output_lock);
                    break;
                }
                if(head->next == NULL){
                    if(flag == 0){
                        pthread_mutex_lock(&output_lock);
                        printf("%*s%s\n",55,"","当前无任务可调度...");
                        pthread_mutex_unlock(&output_lock);
                    }
                    flag = 1;
                    continue;
                }
                else{
                    task_info* cur = head->next;
                    if(cur->remaining_time >= cur_shortest_time && isCpuBusy){
                        continue;
                    }
                    else{
                        flag = 0;
                        sem_getvalue(&(cur->sche),&semVal);
                        if(semVal == 0 && !isCpuBusy && cur->ptid != 0){
                            sem_post(&(cur->sche));
                            cur_task_ptid = cur->ptid;  
                            cur_shortest_time = cur->remaining_time;
                            out(cur);
                            printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                            printf("%*s%s %d\n",55,"","当前剩余时间为",cur->remaining_time);
                        } 
                        else{
                            if(isCpuBusy && cur->ptid != 0){
                                pthread_kill(cur_task_ptid,SIGALRM);                                
                                sem_post(&(cur->sche));
                                cur_task_ptid = cur->ptid;
                                cur_shortest_time = cur->remaining_time;
                                out(cur);
                                printf("%*s%s %ld\n",55,"","进行抢占，线程号为",cur->ptid);
                                printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                                //printf("                                                            调度线程%ld\n",cur->ptid);
                                printf("%*s%s %d\n",55,"","当前剩余时间为",cur->remaining_time);
                                //printf("                                                            当前优先级为%d\n",cur->priority);
                                //pthread_join(cur->ptid,NULL);
                            }
                        }
                    }
                }
            }
            long long* waiting_time;
            long long total_waiting_time = 0;
            for(int i = 0;i < task_num;i++){
                pthread_join(ptid[i],(void**)&waiting_time);
                total_waiting_time += (*waiting_time);
                
                free(waiting_time);
            }
            printf("%*s%s %lld %s\n",55,"","平均等待时间为：",total_waiting_time / task_num,"s");
            //printf("平均等待时间为：%lld ms\n",total_waiting_time / task_num);
            break;
        }

        if(sel == 3){
            int semVal;
            int flag = 0;
            //task_info* cur = head->next;
            while(1){
                if(endFlag){
                    pthread_mutex_lock(&output_lock);
                    printf("%*s%s\n",55,"","结束");
                    pthread_mutex_unlock(&output_lock);
                    break;
                }
                if(head->next == NULL){
                    //cur = head->next;
                    //printf("hello world!");
                    if(flag == 0){
                        pthread_mutex_lock(&output_lock);
                        printf("%*s%s\n",55,"","当前无任务可调度...");
                        pthread_mutex_unlock(&output_lock);
                        //printf("                                                         当前无任务可调度...\n");
                    }
                    flag = 1;
                    continue;
                }
                else{
                    task_info* cur = head->next;
                    if(cur->priority >= cur_highest_priority && isCpuBusy){
                        continue;
                    }
                    else{
                        flag = 0;
                        sem_getvalue(&(cur->sche),&semVal);
                        // if(semVal == 0 && cur->ptid != 0 && !isCpuBusy){
                        if(semVal == 0 && !isCpuBusy && cur->ptid != 0){
                            sem_post(&(cur->sche));
                            cur_task_ptid = cur->ptid;  
                            cur_highest_priority = cur->priority;
                            out(cur);
                            printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                            //printf("                                                            调度线程%ld\n",cur->ptid);
                            printf("%*s%s %d\n",55,"","当前优先级为",cur->priority);
                            //printf("                                                            当前优先级为%d\n",cur->priority);
                            //pthread_join(cur->ptid,NULL);
                        } 
                        else{
                            if(isCpuBusy && cur->ptid != 0){
                                
                                printf("the cur_task_ptid = %ld\n",cur_task_ptid);
                                pthread_kill(cur_task_ptid,SIGALRM);
                                
                                sem_post(&(cur->sche));
                                cur_task_ptid = cur->ptid;
                                cur_highest_priority = cur->priority;
                                out(cur);
                                printf("%*s%s %ld\n",55,"","进行抢占，线程号为",cur->ptid);
                                printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                                //printf("                                                            调度线程%ld\n",cur->ptid);
                                printf("%*s%s %d\n",55,"","当前优先级为",cur->priority);
                                //printf("                                                            当前优先级为%d\n",cur->priority);
                                //pthread_join(cur->ptid,NULL);
                            }
                        }
                    }
                }
            }
            long long* waiting_time;
            long long total_waiting_time = 0;
            for(int i = 0;i < task_num;i++){
                pthread_join(ptid[i],(void**)&waiting_time);
                total_waiting_time += (*waiting_time);
                
                free(waiting_time);
            }
            printf("%*s%s %lld %s\n",55,"","平均等待时间为：",total_waiting_time / task_num,"s");
            //printf("平均等待时间为：%lld ms\n",total_waiting_time / task_num);
            break;
        }
    }
    
    
    pthread_exit(NULL);
}


void* task_generator(){
    printf("选择调度策略：\n1. FCFS\n2. SJB\n3. Priority\n");
    scanf("%d",&sel);
    if(sel == 1){
        int cnt = 1;
        int wct;
        task_info* p;
        while(1){
            pthread_mutex_lock(&output_lock);
            printf("输入第%d个任务的wct(输入-1表示结束任务输入)：",cnt);
            pthread_mutex_unlock(&output_lock);
            cnt++;
            scanf("%d",&wct);
            if(wct == -1){
                endFlag = 1;
                break;
            }
            task_num = cnt - 1;
            p = (task_info*)malloc(sizeof(task_info));
            p->wct = wct;
            p->remaining_time = wct;
            p->stat = READY;
            p->next = NULL;
            tail->next = p;
            tail = p;
            p->next = NULL;
            sem_init(&(p->sche),1,0);
            gettimeofday(&(p->arrive_time),0);
            pthread_create(&(p->ptid), NULL, &task_func, p);
            ptid[cnt-2] = p->ptid;           
        }
    }
    if(sel == 2){
        int cnt = 1;
        int priority;
        int wct;
        task_info* p;
        printf("输入-1表示结束任务输入\n");
        while(1){
            pthread_mutex_lock(&output_lock);
            printf("输入第%d个任务的wct(整数 秒为单位)：",cnt);
            pthread_mutex_unlock(&output_lock);
            scanf("%d",&wct);
            if(wct == -1){
                endFlag = 1;
                break;
            }
            cnt++;
            task_num = cnt - 1;
            p = (task_info*)malloc(sizeof(task_info));
            p->wct = wct;
            p->remaining_time = wct;
            p->stat = READY;
            p->next = NULL;
            insert2(p); // 插入到就绪队列中，按优先级从高到低排列
            // tmp = head->next;
            // while(tmp != NULL){
            //     if(tmp->priority <= priority && tmp->next == NULL){
            //         tmp->next = p;
            //         tail = p;
            //         break;
            //     }
            //     if(tmp->priority <= priority && (tmp->next)->priority > priority){

            //     }
            // }
            sem_init(&(p->sche),1,0);
            gettimeofday(&(p->arrive_time),0);
            pthread_create(&(p->ptid), NULL, &task_func, p);
            pthread_mutex_lock(&output_lock);
            printf("%*s%s  %ld\n",100,"","任务线程被创建",p->ptid);
            pthread_mutex_unlock(&output_lock);
            //printf("%*s%s%*s",10,"","123",3,"" );
            ptid[cnt-2] = p->ptid;           
        }
    }
    if(sel == 3){
        int cnt = 1;
        int priority;
        int wct;
        task_info* p;
        printf("优先级1~10，1最高，10最低，输入-1表示结束任务输入\n");
        while(1){
            pthread_mutex_lock(&output_lock);
            printf("输入第%d个任务的优先级：",cnt);
            pthread_mutex_unlock(&output_lock);
            scanf("%d",&priority);
            if(priority == -1){
                endFlag = 1;
                break;
            }
            pthread_mutex_lock(&output_lock);
            printf("输入第%d个任务的wct(整数 秒为单位)：",cnt);
            pthread_mutex_unlock(&output_lock);
            scanf("%d",&wct);
            cnt++;
            // if(priority < cur_highest_priority){
            //     cur_highest_priority = priority;
            // }
            task_num = cnt - 1;
            p = (task_info*)malloc(sizeof(task_info));
            p->priority = priority;
            p->wct = wct;
            p->remaining_time = wct;
            p->stat = READY;
            p->next = NULL;
            insert(p); // 插入到就绪队列中，按优先级从高到低排列
            // tmp = head->next;
            // while(tmp != NULL){
            //     if(tmp->priority <= priority && tmp->next == NULL){
            //         tmp->next = p;
            //         tail = p;
            //         break;
            //     }
            //     if(tmp->priority <= priority && (tmp->next)->priority > priority){

            //     }
            // }
            sem_init(&(p->sche),1,0);
            gettimeofday(&(p->arrive_time),0);
            pthread_create(&(p->ptid), NULL, &task_func, p);
            pthread_mutex_lock(&output_lock);
            printf("%*s%s  %ld\n",100,"","任务线程被创建",p->ptid);
            pthread_mutex_unlock(&output_lock);
            //printf("%*s%s%*s",10,"","123",3,"" );
            ptid[cnt-2] = p->ptid;           
        }
    }
}
   


int main(){
    int rc;
    pthread_t gen_ptid,sche_ptid;
    head = (task_info*)malloc(sizeof(task_info));
    head->next = NULL;
    tail = head;

    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));  
    sigemptyset(&actions.sa_mask); /* 将参数set信号集初始化并清空 */  
    actions.sa_flags = 0;  
    actions.sa_handler = sighand;
    sigaction(SIGALRM,&actions,NULL); 

    pthread_mutex_init(&head_lock,NULL);
    pthread_mutex_init(&output_lock,NULL);
    sem_init(&isCpuBusy_mutex,1,1);
    pthread_create(&gen_ptid,NULL,&task_generator,NULL);
    pthread_create(&sche_ptid,NULL,&task_schedule,NULL);
    pthread_join(gen_ptid,NULL);
    pthread_join(sche_ptid,NULL);
    free(head);
    
    return 0;
}
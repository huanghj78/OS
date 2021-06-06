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



void insert_priority(task_info* p){
    task_info* cur = head->next;
    task_info* tmp;
    int flag = 0;
    while(cur != NULL){
        flag = 1;
        if(cur == head->next && cur->priority > p->priority){
            head->next = p;
            p->next = cur;
            break;
        }
        // if(cur->next == NULL && cur != head->next){
        if(cur->next == NULL){
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

void insert_remaining_time(task_info* p){
    task_info* cur = head->next;
    task_info* tmp;
    int flag = 0;
    while(cur != NULL){
        flag = 1;
        if(cur == head->next && cur->remaining_time > p->remaining_time){
            head->next = p;
            p->next = cur;
        }
        if(cur->next == NULL){
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

void remove_node(task_info* cur){
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

void sighand(int signo){  
    pthread_t ptid = pthread_self();  
    //printf("Thread %lu in preempted\n", tid);  
    printf("%*s%s%ld%s\n",55,"","调度线程",ptid,"被抢占");
    return;  
}

void *task_func(void* args){
    // 动态申请一块区域存放任务等待时间作为返回参数
    int* waiting_time = (int*)malloc(sizeof(int));
    // 获取传入的指针参数
    task_info* task = (task_info*)args; 
    int flag = 0;
    // 任务因为被抢占的等待时间  = 任务开始时间 - 任务结束时间 - 任务执行时间
    while(1){
        // 尝试执行任务，在被调度之前，由于sche为0，会阻塞于此
        sem_wait(&(task->sche));
        /* 为了避免后面因为被抢占而被再次调度时再次获取开始时间，
        设置一个标志为使其只在一开始获取*/ 
        if(flag == 0){
            // 被调度之后，获取时间作为任务开始时间;
            gettimeofday(&(task->start_time),0);
            flag = 1;
        }        
        // 表示CPU被占用
        isCpuBusy = 1;
        pthread_mutex_lock(&output_lock);
        printf("%*s%s%ld%s\n",100,"","线程号为",task->ptid,"开启");
        pthread_mutex_unlock(&output_lock);
        // 任务状态置为运行状态
        task->stat = RUNNING;
        // 由于sleep()函数被打断时会返回剩余时间，可以用此判断是否被抢占
        task->remaining_time = sleep(task->remaining_time);
        // 剩余时间大于0，说明被抢占
        if(task->remaining_time > 0){
            // 根据所选策略选择相应的插入函数
            if(strategy == 2){
                insert_remaining_time(task);
            }
            else{
                insert_priority(task);
            }
            // 任务状态置为阻塞状态
            task->stat = BLOCKED;
            // 尝试重新执行任务
            continue;
        }
        // 至此任务执行完毕u，获取时间作为任务结束时间
        gettimeofday(&(task->end_time),0); 
        // 等待时间 = 任务开始时间 - 任务到达时间 + （任务结束时间 - 任务开始时间 — 任务执行时间wct）
        *waiting_time = task->start_time.tv_sec - task->arrive_time.tv_sec + task->end_time.tv_sec - task->start_time.tv_sec - task->wct;
        // 释放任务结点资源
        free(task);
        pthread_mutex_lock(&output_lock);
        printf("%*s%s%ld%s\n",100,"","线程号为",task->ptid,"结束");
        pthread_mutex_unlock(&output_lock);
        // 置CPU为空闲状态
        isCpuBusy = 0;
        break;
    }    
    pthread_exit((void*)waiting_time);
}

void* task_schedule(){
    printf("%*s%s%*s%s\n",55,"","任务调度情况",45,"","任务线程情况");
    while(1){ // 此层循环是为等待策略的选择，一旦进入某个策略的代码块，到最后就会break
        // FCFS
        if(strategy == 1){
            int semVal;
            int flag = 0;
            while(1){
                // 结束标志位被置高时
                if(endFlag){
                    printf("%*s%s\n",55,"","调度线程结束");
                    break;
                }
                // 就绪队列中没有任务时
                if(head->next == NULL){
                    if(flag == 0){
                        printf("%*s%s\n",55,"","当前无任务可调度...");
                    }
                    flag = 1;
                    continue;
                }
                // 就绪队列中有任务时
                else{
                    // 由于时FCFS，属于非抢占式，因此当CPU被占用时，便只能等待CPU被释放
                    if(isCpuBusy){
                        continue;
                    }
                    // CPU空闲时
                    else{
                        flag = 0;
                        task_info* cur = head->next;
                        sem_getvalue(&(cur->sche),&semVal);
                        // 由于就绪队列头结点没有存储信息，为避免调度第一个结点的信息，加上cur->ptid != 0
                        //if(semVal == 0 && cur->ptid != 0 && !isCpuBusy){
                        if(cur->ptid != 0){
                            sem_post(&(cur->sche));
                            printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                            remove_node(cur);
                        } 
                    }
                }
            }
            // 调度完毕之后，计算平均等待时间
            long long* waiting_time;
            double total_waiting_time = 0;
            // 获取每个任务线程函数返回的等待时间并全部加起来
            for(int i = 0;i < task_num;i++){
                pthread_join(ptid[i],(void**)&waiting_time);
                total_waiting_time += (*waiting_time);
                free(waiting_time);
            }
            // 防止除以0
            if(task_num == 0){
                break;
            }
            printf("%*s%s %f %s\n",55,"","平均等待时间为：",total_waiting_time / task_num,"s");
            break;
        }
        // SJB
        if(strategy == 2){
            int semVal;
            int flag = 0;
            while(1){
                // 就绪队列中没有任务时
                if(endFlag){
                    pthread_mutex_lock(&output_lock);
                    printf("%*s%s\n",55,"","结束");
                    pthread_mutex_unlock(&output_lock);
                    break;
                }
                // 就绪队列中没有任务时
                if(head->next == NULL){
                    if(flag == 0){
                        pthread_mutex_lock(&output_lock);
                        printf("%*s%s\n",55,"","当前无任务可调度...");
                        pthread_mutex_unlock(&output_lock);
                    }
                    flag = 1;
                    continue;
                }
                // 就绪队列中有任务时
                else{
                    task_info* cur = head->next;
                    // 若当前CPU被占用且当前队首的任务剩余时间少于执行的任务的剩余时间，则无法抢占
                    if(cur->remaining_time >= cur_shortest_time && isCpuBusy){
                        continue;
                    }
                    else{
                        flag = 0;
                        sem_getvalue(&(cur->sche),&semVal);
                        // CPU空闲，则正常调度即可
                        if(semVal == 0 && !isCpuBusy && cur->ptid != 0){
                            sem_post(&(cur->sche));
                            // 更新当前信息
                            cur_task_ptid = cur->ptid;  
                            cur_shortest_time = cur->remaining_time;
                            // 将执行的任务移出就绪队列
                            remove_node(cur);
                            printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                            printf("%*s%s %d\n",55,"","当前剩余时间为",cur->remaining_time);
                            /* 由于任务线程进入工作状态设置相应标志位需要一定时间，
                            而while循环很快，为了避免有多个任务被同时调度，
                            在此睡眠50ms */
                            usleep(50000);
                        } 
                        // CPU已被占用，则发生抢占
                        else{
                            if(isCpuBusy && cur->ptid != 0){
                                // 先当前执行的任务线程发送信号，使其停止工作（睡眠），进入堵塞状态
                                pthread_kill(cur_task_ptid,SIGALRM);                                
                                // 调度队首任务
                                sem_post(&(cur->sche));
                                // 更新当前信息
                                cur_task_ptid = cur->ptid;
                                cur_shortest_time = cur->remaining_time;
                                // 将执行的任务移出就绪队列
                                remove_node(cur);
                                printf("%*s%s %ld\n",55,"","进行抢占，线程号为",cur->ptid);
                                printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                                printf("%*s%s %d\n",55,"","当前剩余时间为",cur->remaining_time);
                            }
                        }
                    }
                }
            }
            // 调度完毕之后，计算平均等待时间
            long long* waiting_time;
            double total_waiting_time = 0;
            for(int i = 0;i < task_num;i++){
                pthread_join(ptid[i],(void**)&waiting_time);
                total_waiting_time += (*waiting_time);               
                free(waiting_time);
            }
            if(task_num == 0){
                break;
            }
            printf("%*s%s %f %s\n",55,"","平均等待时间为：",total_waiting_time / task_num,"s");
            break;
        }
        // 优先级调度
        if(strategy == 3){
            int semVal;
            int flag = 0;
            while(1){
                // 结束标志被置高
                if(endFlag){
                    pthread_mutex_lock(&output_lock);
                    printf("%*s%s\n",55,"","结束");
                    pthread_mutex_unlock(&output_lock);
                    break;
                }
                // 就绪队列无任务
                if(head->next == NULL){
                    if(flag == 0){
                        pthread_mutex_lock(&output_lock);
                        printf("%*s%s\n",55,"","当前无任务可调度...");
                        pthread_mutex_unlock(&output_lock);
                    }
                    flag = 1;
                    continue;
                }
                // 就绪队列有任务
                else{
                    task_info* cur = head->next;
                    // 若当前CPU被占用且当前队首的优先级低于或等于执行的任务的优先级，则无法抢占
                    if(cur->priority >= cur_highest_priority && isCpuBusy){
                        continue;
                    }
                    else{
                        flag = 0;
                        sem_getvalue(&(cur->sche),&semVal);
                        // CPU空闲，则正常调度即可
                        if(semVal == 0 && !isCpuBusy && cur->ptid != 0){
                            sem_post(&(cur->sche));
                            cur_task_ptid = cur->ptid;  
                            cur_highest_priority = cur->priority;
                            // 将执行的任务移出就绪队列
                            remove_node(cur);
                            printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                            printf("%*s%s %d\n",55,"","当前优先级为",cur->priority);
                            usleep(50000);
                        } 
                        // CPU被占用，则发生抢占
                        else{
                            if(isCpuBusy && cur->ptid != 0){   
                                // 先当前执行的任务线程发送信号，使其停止工作（睡眠），进入堵塞状态                            
                                pthread_kill(cur_task_ptid,SIGALRM);
                                // 调度队首任务                                
                                sem_post(&(cur->sche));
                                // 更新当前状态
                                cur_task_ptid = cur->ptid;
                                cur_highest_priority = cur->priority;
                                remove_node(cur);
                                printf("%*s%s %ld\n",55,"","进行抢占，线程号为",cur->ptid);
                                printf("%*s%s %ld\n",55,"","调度线程",cur->ptid);
                                printf("%*s%s %d\n",55,"","当前优先级为",cur->priority);
                            }
                        }
                    }
                }
            }
            // 调度完毕之后，计算平均等待时间
            long long* waiting_time;
            double total_waiting_time = 0;
            for(int i = 0;i < task_num;i++){
                pthread_join(ptid[i],(void**)&waiting_time);
                total_waiting_time += (*waiting_time);                
                free(waiting_time);
            }
            if(task_num == 0){
                break;
            }
            printf("%*s%s %f %s\n",55,"","平均等待时间为：",total_waiting_time / task_num,"s");
            break;
        }
    }    
    pthread_exit(NULL);
}


void* task_generator(){
    printf("选择调度策略：\n1. FCFS\n2. SJB\n3. Priority\n");
    scanf("%d",&strategy);
    // 调度策略1
    if(strategy == 1){
        int cnt = 1;    // 任务计数
        int wct;    
        task_info* p;
        printf("输入-1表示结束任务输入，wct数值为整数，单位为秒，\n");
        while(1){
            // 互斥输出
            pthread_mutex_lock(&output_lock);
            printf("输入第%d个任务的wct：",cnt);
            pthread_mutex_unlock(&output_lock);
            cnt++;
            scanf("%d",&wct);
            if(wct == -1){
                endFlag = 1;
                break;
            }
            task_num = cnt - 1;
            // 创建新任务结点并对其中的变量进行初始化
            p = (task_info*)malloc(sizeof(task_info));
            p->wct = wct;
            p->remaining_time = wct;
            p->stat = READY;
            p->next = NULL;
            tail->next = p;
            sem_init(&(p->sche),1,0);
            // 将结点插入到队尾
            tail = p;
            p->next = NULL;
            // 获取任务到达时间
            gettimeofday(&(p->arrive_time),0);
            // 为该任务创建线程
            pthread_create(&(p->ptid), NULL, &task_func, p);
            pthread_mutex_lock(&output_lock);
            printf("%*s%s  %ld\n",100,"","任务线程被创建",p->ptid);
            pthread_mutex_unlock(&output_lock);
            ptid[cnt-2] = p->ptid;           
        }
    }
    // 调度策略2
    if(strategy == 2){
        int cnt = 1;
        int priority;
        int wct;
        task_info* p;
        printf("输入-1表示结束任务输入，wct数值为整数，单位为秒，\n");
        while(1){
            pthread_mutex_lock(&output_lock);
            printf("输入第%d个任务的wct：",cnt);
            pthread_mutex_unlock(&output_lock);
            scanf("%d",&wct);
            if(wct == -1){
                endFlag = 1;
                break;
            }
            cnt++;
            task_num = cnt - 1;
            // 创建新任务结点并对其中的变量进行初始化
            p = (task_info*)malloc(sizeof(task_info));
            p->wct = wct;
            p->remaining_time = wct;
            p->stat = READY;
            p->next = NULL;
            sem_init(&(p->sche),1,0);
            // 插入到就绪队列中，按剩余时间从少到多排列
            insert_remaining_time(p);            
            // 获取任务到达时间
            gettimeofday(&(p->arrive_time),0);
            // 为该任务创建线程
            pthread_create(&(p->ptid), NULL, &task_func, p);
            pthread_mutex_lock(&output_lock);
            printf("%*s%s  %ld\n",100,"","任务线程被创建",p->ptid);
            pthread_mutex_unlock(&output_lock);
            ptid[cnt-2] = p->ptid;           
        }
    }
    // 调度策略3
    if(strategy == 3){
        int cnt = 1;
        int priority;
        int wct;
        task_info* p;
        printf("优先级1~10，1最高，10最低，输入-1表示结束任务输入\n");
        printf("wct数值为整数，单位为秒\n");
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
            printf("输入第%d个任务的wct：",cnt);
            pthread_mutex_unlock(&output_lock);
            scanf("%d",&wct);
            cnt++;
            task_num = cnt - 1;
            // 创建新任务结点并对其中的变量进行初始化
            p = (task_info*)malloc(sizeof(task_info));
            p->priority = priority;
            p->wct = wct;
            p->remaining_time = wct;
            p->stat = READY;
            p->next = NULL;
            sem_init(&(p->sche),1,0);
            // 插入到就绪队列中，按优先级从高到低排列
            insert_priority(p); 
            // 获取任务到达时间   
            gettimeofday(&(p->arrive_time),0);
            // 为该任务创建线程
            pthread_create(&(p->ptid), NULL, &task_func, p);
            pthread_mutex_lock(&output_lock);
            printf("%*s%s  %ld\n",100,"","任务线程被创建",p->ptid);
            pthread_mutex_unlock(&output_lock);
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

    pthread_mutex_init(&output_lock,NULL);
    pthread_create(&gen_ptid,NULL,&task_generator,NULL);
    pthread_create(&sche_ptid,NULL,&task_schedule,NULL);
    pthread_join(gen_ptid,NULL);
    pthread_join(sche_ptid,NULL);
    free(head);
    
    return 0;
}
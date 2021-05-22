#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define STACK_SIZE 1024*1024
static char child_stack[STACK_SIZE];
int num = 1;
static int child_fn() {
    //  printf("child process PID: %d\n",getpid());
    //  printf("child process  parent PID: %d\n",getppid());
    // printf("In the child process: num = %d,the address of num = %p\n",num,&num);
    // printf("change the value of num...\n");
    // num = 2;
    // printf("In the child process: num = %d,the address of num = %p\n",num,&num);
     long int randomcount;
     for(int i = 0;i < 5;i++){
         randomcount = rand() % 1000000;
        for (int k =0; k < randomcount; k++) ;
         printf("                             I am child process,i = %d\n",i);
         //sleep(1);
     }
     return 0;
}

int main() {
     long int randomcount;
    //  printf("calling process PID: %d\n",getpid());
    //  printf("calling process parent PID: %d\n",getppid());
    //printf("In the main process(before calling child process): num = %d,the address of num = %p\n",num,&num);
     pid_t child_pid = clone(child_fn, child_stack+STACK_SIZE, CLONE_VM| CLONE_VFORK|SIGCHLD, NULL);
        for(int i = 0;i < 5;i++){
            randomcount = rand() % 1000000;
        for (int k =0; k < randomcount; k++) ;
         printf("I am main process,i = %d\n",i);
         //sleep(1);
     }
        waitpid(child_pid, NULL, 0);
    //printf("In the main process(after calling child process): num = %d,the address of num = %p\n",num,&num);
     
     return 0;
}
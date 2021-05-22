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
    printf("In the child process: num = %d\n",num);
    printf("change the value of num...\n");
    num = 2;
    printf("In the child process: num = %d\n",num);
     return 0;
}

int main() {
     
    //  printf("calling process PID: %d\n",getpid());
    //  printf("calling process parent PID: %d\n",getppid());
    printf("In the main process(before calling child process): num = %d\n",num);
     pid_t child_pid = clone(child_fn, child_stack+STACK_SIZE, CLONE_VM| SIGCHLD, NULL);
        waitpid(child_pid, NULL, 0);
    printf("In the main process(after calling child process): num = %d\n",num);
     
     return 0;
}
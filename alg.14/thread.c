#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#define gettid() syscall(__NR_gettid)
#define STACK_SIZE 1024*1024
static char child_stack[STACK_SIZE];

static int child_fn1() {
     printf("child process1 PID: %d\n",getpid());
     printf("child process1 TID: %ld\n",gettid());
     printf("child process1  parent PID: %d\n",getppid());

     printf("execute hello.c\n");
     char* buf[50];
     execv("./hello.o",buf);
    printf("child process 1 will be terminated\n");
     return 0;
}

static int child_fn2() {
     printf("child process2 PID: %d\n",getpid());
     printf("child process2 TID: %ld\n",gettid());
     printf("child process2  parent PID: %d\n",getppid());
     sleep(5);
    printf("child process 2 will be terminated\n");
     return 0;
}

int main() {
     printf("calling process PID: %d\n",getpid());
     printf("calling process TID: %ld\n",gettid());
     printf("calling process parent PID: %d\n",getppid());
     pid_t child_pid1 = clone(child_fn1, child_stack+STACK_SIZE, /*CLONE_THREAD|*/CLONE_SIGHAND|CLONE_VFORK|CLONE_VM| SIGCHLD, NULL);
     if(child_pid1 == -1) {
        perror("clone()");
        exit(1);
    }
    pid_t child_pid2 = clone(child_fn2, child_stack+1048576, /*CLONE_THREAD|*/CLONE_SIGHAND|CLONE_VFORK|CLONE_VM| SIGCHLD, NULL);
     if(child_pid2 == -1) {
        perror("clone()");
        exit(1);
    }
     waitpid(child_pid1, NULL, 0);
     waitpid(child_pid2, NULL, 0);
     printf("calling process will be terminated\n");
     return 0;
}
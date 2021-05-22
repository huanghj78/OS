#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define STACK_SIZE 1024*1024
static char child_stack[STACK_SIZE];

static int child_fn() {
     printf("child process PID: %d\n",getpid());
     printf("child process  parent PID: %d\n",getppid());
     return 0;
}

int main() {
     printf("calling process PID: %d\n",getpid());
     printf("calling process parent PID: %d\n",getppid());
     pid_t child_pid = clone(child_fn, child_stack+STACK_SIZE, CLONE_PARENT| SIGCHLD, NULL);

     waitpid(child_pid, NULL, 0);
     return 0;
}
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define STACK_SIZE 1024*1024
static char child_stack[STACK_SIZE];

static int child_fn() {
    char command[50];
    printf("This is the child process\n");
    sprintf(command,"ipcs");
    system(command);
    return 0;
}

int main() {
    char command[50];
     pid_t child_pid = clone(child_fn, child_stack+STACK_SIZE, /*CLONE_NEWIPC|*/SIGCHLD, NULL);
     waitpid(child_pid, NULL, 0);
     sprintf(command,"ipcs");
     printf("This is the main process\n");
    system(command);
     return 0;
}
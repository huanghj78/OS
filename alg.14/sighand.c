#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#define STACK_SIZE 1024*1024
static char child_stack[STACK_SIZE];

static int child_fn() {
		printf("I am the child process. PID: %d\n", getpid());
		sleep (100);
		return 0;
}

static void hdl (int sig, siginfo_t *siginfo, void *context)
{
     printf ("Here is handle! PID = %d\n",getpid());
     //  输出所在进程的ＩＤ
}

int main() {
     struct sigaction act;
    
     printf("I am the parent process. PID: %d\n", getpid());
     pid_t child_pid = clone(child_fn, child_stack+STACK_SIZE, CLONE_VM /*| CLONE_SIGHAND*/ | SIGCHLD, NULL);

     memset (&act, '\0', sizeof(act));

     act.sa_sigaction = &hdl;
     act.sa_flags = SA_SIGINFO;

     if (sigaction(SIGINT, &act, NULL) < 0) {
				perror ("sigaction");
             return 1;
     }

     waitpid(child_pid, NULL, 0);
     printf("child terminated!\n");
     return 0;
}
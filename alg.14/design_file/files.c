#define _GNU_SOURCE  
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define STACK_SIZE	65536
int fd;

static int child_fn() {
    char command[50];
  printf("\nChild process: before closing the file\n");
  printf("=======================\n");
  sprintf(command,"lsof -p %d",getpid());
  system(command);
  close(fd);

  printf("\nChild process: after closing the file\n");
  printf("=======================\n");
  system(command);
}

int main(int argc, char *argv[])
{
	//Allocate stack for child task
	char *stack = malloc(STACK_SIZE);
    char command[50];
	if (!stack) {
		perror("Failed to allocate memory\n");
        exit(1);
	}

    fd = open("file.txt", O_RDWR);
	if (fd == -1) {
        perror("Failed to open file\n");
		exit(1);
	}
    printf("\nParent process: before calling child process\n");
    printf("=======================\n");
    sprintf(command,"lsof -p %d",getpid());
    system(command);
    pid_t child_pid = clone(child_fn, stack + STACK_SIZE, CLONE_FILES | SIGCHLD, NULL);

	
    waitpid(child_pid, NULL, 0);
    printf("\nParent process: after calling child process\n");
    printf("=======================\n");
    printf("This is parent process\n");
    sprintf(command,"lsof -p %d",getpid());
    system(command);

	return 0;
}
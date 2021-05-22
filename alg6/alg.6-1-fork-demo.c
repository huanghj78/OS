#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int count = 1;
    pid_t childpid;
    
    childpid = fork(); /* child duplicates parent’s address space */
    if (childpid < 0) {
        perror("fork()");
        return EXIT_FAILURE;
    }
    else /* fork() returns 2 values: 0 for child pro and childpid for parent pro */
        if (childpid == 0) { /* This is child pro */
            count++;
	    int test = 1;
            printf("Child pro pid = %d, count = %d (addr = %p), test = %d (addr = %p)\n", getpid(), count, &count,test, &test);
        }
        else { /* This is parent pro */
	    int test = 2;
            printf("Parent pro pid = %d, child pid = %d, count = %d (addr = %p), test = %d (addr = %p)\n", getpid(), childpid, count, &count,test,&test);
            sleep(5);
            wait(0); /* waiting for all children terminated */
        }
    printf("Testing point by %d\n", getpid()); /* child executed this statement and became defunct before parent wait() */
    return EXIT_SUCCESS;
}

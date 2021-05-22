#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

int gap;

static void *ftn(void *arg)
{
    int *s_addr = (int *)arg;
    int num = *s_addr;
    int i,sum = 0;
    int tmp = gap;
    int* retptr;
    for(i = num+1;tmp > 0;i++){
        tmp--;
        sum += i;
    }
    retptr = (int*)malloc(sizeof(int));
    *retptr = sum;
    //printf("i am thread number %d, ptid = %ld\n", num, pthread_self());
    pthread_exit((void*)retptr);
}
 
int pthread_demo(int max,int order) /* create some pthread threads */ 
{
    int ret;
    int res[order];
    int num[order];
    int result = 0;
    int *retptr;
    gap = max / order;//

    for (int i = 0; i < order; i++) {
        num[i] = gap * i;
    }

    long start_us, end_us;
    struct timeval t;

    // gettimeofday(&t, 0);
    // start_us = (long)(t.tv_sec * 1000 * 1000) + t.tv_usec;

    // sleep(2);

    // gettimeofday(&t, 0);
    // end_us = (long)(t.tv_sec * 1000 * 1000) + t.tv_usec;

    // printf("time testing... sleeping time = %ld micro sec\n", end_us - start_us);
    	    
    gettimeofday(&t, 0);
    start_us = (long)(t.tv_sec * 1000 * 1000) + t.tv_usec;

    pthread_t ptid[order];
    for (int i = 0; i < order; i++) {
        ret = pthread_create(&ptid[i], NULL, &ftn, (void *)&num[i]);
        if(ret != 0) {
            fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
            exit(1);
        }
    }

    for (int i = 0; i < order; i++) {
        ret = pthread_join(ptid[i], (void**)&retptr);
        res[i] = *retptr;
        free(retptr);
        if(ret != 0) {
            fprintf(stderr, "pthread_join error: %s\n", strerror(ret));
            exit(1);
        }
    }

    for(int i = 0; i < order;i++){
        result += res[i];
    }

    gettimeofday(&t, 0);
    end_us = (long)(t.tv_sec * 1000 * 1000) + t.tv_usec;
    printf("with %d threads, running time = %ld usec\n", order, end_us - start_us);

    return result;
}

int main(){
    int max,num,res;
    printf("add 1 to max , max = ");
    scanf("%d",&max);
    printf("how many threads :");
    scanf("%d",&num);
    res = pthread_demo(max,num);
    printf("result = %d\n",res);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

static void* ftn(void* arg){
    int* retptr = (int*)malloc(sizeof(int));
    int *tmp = (int*)arg;
    int input = *tmp;
    printf("This is the sub thread, the num from main thread is %d\nInput the num wanna pass to main thread:",input);
    scanf("%d",retptr);
    pthread_exit((void*)retptr);
}

int main(){
    int ret,input,output;
    int* retptr;
    pthread_t ptid;
    printf("This is th main. Input the num wanna pass to main thread:");
    scanf("%d",&input);
    ret = pthread_create(&ptid,NULL,&ftn,&input);
    if(ret == -1){
        perror("pthread_creat()");
    }
    ret = pthread_join(ptid,(void**)&retptr);
    if(ret == -1){
        perror("pthread_join()");
    }
    printf("from sub: %d\n",*retptr);
    return 0;


}
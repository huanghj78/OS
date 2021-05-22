#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

static void* runner(void* arg){
    int* tmp = arg;
    int num = *tmp;
    char* retptr = (char*)malloc(sizeof(20));
    sprintf(retptr,"This is No. %d thread!\n",num);
    pthread_exit((void*)retptr);
}

int main(){
    pthread_t ptid[5];
    pthread_attr_t attr;
    int ret,i;
    char* retptr;
    int pthread_num[5];
    pthread_attr_init(&attr);

    for(i = 0;i < 5;i++){
        pthread_num[i] = i;
    }

    for(i = 0;i < 5;i++){
        ret = pthread_create(&ptid[i],&attr,&runner,&pthread_num[i]);
        if(ret != 0){
            perror("pthread_creat()");
            return 1;
        }
    }

    for(i = 0;i < 5;i++){
        printf("%ld\n",ptid[i]);
        ret = pthread_join(ptid[i],(void**)&retptr);
        if(ret != 0){
            perror("pthread_join()");
            return 1;
        }
        printf("%s\n",retptr);
        free(retptr);
    }

    return 0;
}
#include <sys/time.h>
#include <stdio.h>
int main(){
    struct timeval t;
    
    while(1){
        gettimeofday(&t,0);
        printf("%ld        ",t.tv_sec);
        printf("%ld\n",t.tv_usec);
    }
    
    return 0;
}
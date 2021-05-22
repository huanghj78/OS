#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MIN 1
#define MAX 100
void swap(int arr[],int i,int j){//实现对堆指定的两个元素进行交换
	int tmp = arr[i];
	arr[i] = arr[j];
	arr[j] = tmp;
}

void percolate(int arr[],int start,int end){//对堆从第start个元素到第end个元素进行下滤
	int tmp1,tmp2;
	int i = start;
	tmp1 = i;
	tmp2 = tmp1 << 1;
	while(tmp2 <= end){
		if((tmp2|1) <= end){//若右儿子不超过指定范围
			if(arr[tmp2] < arr[tmp1] || arr[tmp2|1] < arr[tmp1]){//若存在一个儿子比该元素小
				if(arr[tmp2] < arr[tmp2|1]){//选择较小的儿子与该元素进行交换
					swap(arr,tmp1,tmp2);
                    tmp1 = tmp2;
		            tmp2 = tmp1 << 1;
				}
				else{
					swap(arr,tmp1,tmp2|1);
                    tmp1 = tmp2|1;
		            tmp2 = tmp1 << 1;
				}
			}
			else{
				break;
			}
		}
		else{//若右儿子不超过指定范围，则只需要考虑左儿子
			if(arr[tmp2] < arr[tmp1]){
				swap(arr,tmp1,tmp2);
                tmp1 = tmp2;
		        tmp2 = tmp1 << 1;
			}
			else{
				break;
			}
		}
				
	}	
}
void build_min_heap(int arr[],int n){//建立小顶堆
	int i;
	for(i = n / 2;i > 0;i--){//从底层往上第一个非叶子节点开始逐个向下过滤
		percolate(arr,i,n);	
	}
}
int pop(int arr[],int n){//输出堆顶元素，同时把最后一个元素复制到堆顶并将最后一个元素置为-1，代表空，然后从1到n-1进行一次向下过滤
	int pop = arr[1];
	arr[1] = arr[n];
	arr[n] = -1;
	percolate(arr,1,n-1);
	return pop;
}
void push(int arr[],int n,int num){//向堆中输入一个元素
	int i = n,tmp1,tmp2;
	while(arr[i] == -1){//找到第一个空位置
		i--;
	}
	arr[i+1] = num;
	tmp1 = i+1;
	tmp2 = tmp1 >> 1;
	while(tmp2 > 0){//从插入的位置开始，向堆顶进行一次向上过滤来维护插入后的小顶堆
		if(arr[tmp1] < arr[tmp2]){
			swap(arr,tmp1,tmp2);
		}
		tmp1 = tmp2;
		tmp2 = tmp1 >> 1;
	}
	
}

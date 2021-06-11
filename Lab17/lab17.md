# LabWeek17 实验报告
## 原理性解释
### FIFO
* 先进先出算法，即每次置换最先调入内存的页面，即将内存中等待时间最长的页面进行置换。
* 实现比较简单，可以用一个队列管理内存中的页或者记录每个页进入内存的时间。但相对性能较差，即相同的内存大小和测试序列下，缺页数可能多于LRU算法和CLOCK算法。
* FIFO算法还可能出现Belady异常，即当所分配的物理块数增大而页故障数不减反增的异常现象，而LRU和CLOCK算法均不会出现。
### LRU
* 最近最久未使用算法，即每次置换最近最久未使用的页面。
* 实现比较复杂，最常用的基于栈以及基于矩阵的实现方法都较为复杂，但其性能较好。
### Second chance
* 也称CLOCK算法，通过将内存组织成一个环形队列，在内存中每个页有一个标志位use，每次将指针指向的第一个use标志位为0的页换出，若指向的页的为use标志位为1，则将其置为0，并指向下一个页，故给了该页一次机会，在下一趟指向它之前若又被用到则use标志位可以被重新置为1从而避免被换出。
* 可以看到，CLOCK算法也类似于其找一个最近最少使用的页将其换出，因此它被认为是LRU的近似算法，性能上接近于LRU算法，而实现上比LRU算法简单。

## 细节性解释
### 一些全局变量及相关函数
```c
#include <stdio.h>
#include <stdlib.h>
#define MAX_SIZE 100 

int memory[MAX_SIZE];   // 内存空间
int capacity;           // 内存空间容量
int size;               // 内存空间当前大小
int head;               // FIFO中的队首指针
int testCase[MAX_SIZE];     // 存储测试序列
int matrix[MAX_SIZE][MAX_SIZE] = {0};   
int flag[MAX_SIZE] = {0};   // 表示矩阵中的该行是否有效，即对应的页是否在内存中，因为测例的页号可能不是连续的
// 双向链表结点
typedef struct{
    int pageNum;
    int memIndex;
    struct node* next;
    struct node* last;
}node;
node* top = NULL;
node* buttom = NULL;
int useFlag[MAX_SIZE] = {0};    // second chance中的use标志位
int pointer = 0;                // second chance中的next frame poiter

// 判断当前页号page是否在内存中
int find_page(int page){
    for(int i = 0;i < size;i++){
        if(memory[i] == page){
            return 1;
        }
    }
    return 0;
}
```
### FIFO
* 实现代码如下：
    ```c
    // 先进先出策略，返回缺页数，0或1
    int fifo(int page){
        // 存在内存中
        if(find_page(page)){
            printf("页%d在内存中找到，无需置换\n",page);
            return 0;
        }
        // 不存在内存中
        else{
            // 内存未满，直接放入
            if(size < capacity){
                printf("内存未满，页%d被插入到内存\n",page);
                memory[size] = page;
                size++;
            }
            // 内存已满，需要置换
            else{
                printf("内存已满，页%d被换出，页%d被换入\n",memory[head],page);
                memory[head] = page;
                head = (head + 1 )%capacity;
            }
            return 1;
        } 
    }
    ```
* FIFO的实现是直接将内存看成一个环形队列，通过一个队首指针指向当前的队首元素，即最先进入的页，再下一次置换时便将其换出。
* 由于在整个程序中，内存是用一个静态数组来进行模拟，因此队首指针实际上就是一个整型变量，即队首元素的下标，其移动就是通过```加一之后模上内存容量```，从而实现将内存看成一个环形队列。
* 当内存未满时，则只需要按顺序将页放入到内存中；当内存满时，则通过队首指针找到将被换出的页，用换入的页替换它，然后移动队首指针指向新的最先进入的页。
* 可以看到，FIFO算法的实现是比较简单的，只需要维护一个整型变量，即队首指针即可。
* 同时，由于此算法实际上算是没有策略的策略，它没有考虑到页的使用频率等因素，因此自然地性能不高。

### 基于栈的LRU
* 实现代码如下：
    ```c
    // 基于stack的LRU策略
    int LRU_stack(int page){
        if(find_page(page)){
            printf("页%d在内存中找到，无需置换\n",page);
            node* cur = top;
            node* former;
            node* latter;
            // 找到该页在栈中的位置，将其换到栈顶
            while(1){
                if(cur->pageNum == page){
                    break;
                }
                cur = cur->next;
            }
            // 如果已经在栈顶，则无需再操作
            if(cur == top){
                return 0;
            }
            // 如果在栈底
            if(cur == buttom){
                former = cur->last;
                former->next = NULL;
                cur->last = NULL;
                cur->next = top;
                top->last = cur;
                top = cur;
                buttom = former;
            }
            // 在栈顶和栈底之间
            else{
                former = cur->last;
                latter = cur->next;
                former->next = latter;
                latter->last = former;
                top->last = cur;
                cur->last = NULL;
                cur->next = top;
                top = cur;
            }
            return 0;
        }
        // 缺页
        else{
            if(size < capacity){
                printf("内存未满，页%d被插入到内存\n",page);
                memory[size] = page;
                node* cur = (node*)malloc(sizeof(node));
                // 若栈中无其他元素
                if(top == NULL){
                    cur->pageNum = page;
                    cur->memIndex = size;
                    top = cur;
                    buttom = cur;
                    cur->next = NULL;
                    cur->last = NULL;
                }
                else{
                    cur->pageNum = page;
                    cur->memIndex = size;
                    cur->next = top;
                    top->last = cur;
                    cur->last = NULL;
                    top = cur;
                }       
                size++;
            }
            // 将栈底换出
            else{
                printf("内存已满，页%d被换出，页%d被换入\n",buttom->pageNum,page);
                int index = buttom->memIndex;
                memory[buttom->memIndex] = page;
                node* cur = buttom;
                buttom = buttom->last;
                buttom->next = NULL;
                // 释放栈底元素
                free(cur);
                cur = (node*)malloc(sizeof(node));
                cur->pageNum = page;
                cur->memIndex = index;
                cur->next = top;
                top->last = cur;
                cur->last = NULL;
                top = cur;
            }
            return 1;
        }
    }
    ```
* 通过一个类似于栈的结构（实际上不同于栈）管理内存中的页号，每一次涉及到页的使用时便把该次被使用的页的页号移到栈顶。
* 若内存未满，则只需要将放入内存中的页的页号放在栈的栈顶即可；若内存已满，则选择栈底的页号对应的页换出，然后删除栈底的该元素，再把换入内存的页的页号放入栈顶。
* 通过这样的维护，每次栈底的页号就是最近最少使用的页，因为两个原因，一是较先进入内存中的页的页号会放在较底部，二是被使用的页的页号会被提升到栈顶，因此留在底部的就是最近最少使用的页。
* 由于需要不断维护栈的内部结构，因此可以用一个双向链表对元素进行组织管理，以方便栈中元素的移动。元素的移动只需要根据其前向指针和后向指针找到前后的元素（如果有的话），然后修改它们的指针指向即可。
* 双向链表的涉及到指针的修改较多，比较容易出错，因此实现相对于FIFO只需要维护一个变量要难，带来的好处就是性能较好。

### 基于矩阵的LRU
* 实现代码如下：
    ```c
    int LRU_matrix(int page){
        if(find_page(page)){
            printf("页%d在内存中找到，无需置换\n",page);
            // 更新矩阵
            for(int i = 0;i < MAX_SIZE;i++){
                matrix[page][i] = 1;
                matrix[i][page] = 0;
            }
            return 0;
        }
        else{
            if(size < capacity){
                printf("内存未满，页%d被插入到内存\n",page);
                flag[page] = 1; // 对应标志位置一，表示该页号有效，存在内存中
                memory[size] = page;// 插入
                size++;
                // 更新矩阵
                for(int i = 0;i < MAX_SIZE;i++){
                    matrix[page][i] = 1;
                    matrix[i][page] = 0;
                }
            }
            // 内存已满，进行置换
            else{
                int minPage;
                int min = MAX_SIZE;
                int cnt;
                int index;
                // 找出和最小且存在于内存中的行号，即为将要被置换出去的页号
                for(int i = 0;i < MAX_SIZE;i++){
                    // 不存在内存中，直接跳过
                    if(flag[i] == 0){
                        continue;
                    }
                    cnt = 0;
                    for(int j = 0;j < MAX_SIZE;j++){
                        cnt += matrix[i][j];
                    }
                    if(cnt < min){
                        minPage = i;
                        min = cnt;
                    }
                }
                // 找出要换出的页号在内存中的位置
                for(int i = 0;i < size;i++){
                    if(memory[i] == minPage){
                        index = i;
                        break;
                    }
                }
                printf("内存已满，页%d被换出，页%d被换入\n",minPage,page);
                memory[index] = page;// 置换
                flag[minPage] = 0;// 置零，表示该页号不在内存中
                flag[page] = 1;// 置一，表示该页号在内存中
                // 更新矩阵
                for(int i = 0;i < MAX_SIZE;i++){
                    matrix[page][i] = 1;
                    matrix[i][page] = 0;
                }
            }
            return 1;
        } 
    }
    ```
* 通过维护一个特征矩阵，矩阵的行号和列号对应的就是页号，每次当一个页号为i的页被使用，就让矩阵中的第i行所有元素置为1，再让矩阵中的第i列的所有元素置为0，要置换的时候就是选择一行上所有元素相加之和最小的那一行对应的页换出。
* 可以看到，此方法的实现相较于基于栈的实现较为简单，每次只需要更新矩阵以及找出和最小的一行。但代价是耗费的空间较大，由于页号对应行列号，故理论上来说，矩阵的维度就是最大的页号。
* 而且，对于一个测例，很有可能出现请求的页号并非连续的，即此时有一些页号并不会出现在内存中，当矩阵中有对应其的行列号。为了避免误认为存在于内存中的页，我的解决方法是在定义一个标志位数组，即每个页号对应一个标志位，当其处于内存中时，标志位置为1，否则置为0。这样就可以避免在计算和最小的行时找到一个不存在内存中，即无效的页。
* 由于此算法若作为软件实现，所需内存空间确实是比较大，而且每次需要遍历矩阵找出和最小的行号，当矩阵维度较高时，耗费的时间也是比较多的，因此此方法通常是硬件层面的实现。

### Second chance
* 实现代码如下：
    ```c
    // Second chance策略
    int clock(int page){
        if(find_page(page)){
            printf("页%d在内存中找到，无需置换\n",page);
            // 找到该页号的位置并将其use标志位置一
            for(int i = 0;i < size;i++){
                if(memory[i] == page){
                    useFlag[i] = 1;
                    break;
                }
            }
            return 0;
        }
        else{
            // 内存未满，直接插入
            if(size < capacity){
                printf("内存未满，页%d被插入到内存\n",page);
                memory[size] = page;
                useFlag[size] = 1;
                size++;
            }
            // 内存已满，进行置换
            else{
                // 找到第一个use位为0的页号作为被换出的页
                while(1){
                    // 如果是1，置为0
                    if(useFlag[pointer] == 1){
                        useFlag[pointer] = 0;
                        pointer = (pointer + 1)%capacity; 
                    }
                    else{
                        break;
                    }
                }
                printf("内存已满，页%d被换出，页%d被换入\n",memory[pointer],page);
                memory[pointer] = page;// 置换
                useFlag[pointer] = 1;// 换入的页对应的use标志位置1
                pointer = (pointer + 1) % capacity; 
            }
            return 1;
        } 
    }
    ```
* 此算法的实现只需要在FIFO的基础上为内存中的每一个页增加一个use标志位，将队首指针换为一个指向下一个要判断的页的指针（实际上仍然是一个整型变量），因此只需要额外定义一个数组存放use标志位。
* 当内存未满时，将页放入内存，并将该页对应的use标志位置1，同时移动指针。
* 当内存满时，则判断指针指向的该页的use标志位，若为0则选择该页作为换出页，若为1则将其置0，然后指针移动判断下一个页，直到找到一个可以作为换出页。
* 可以看到，此算法的实现难度介于FIFO和LRU之间，而其做到了类似LRU的策略，因此性能也是介于FIFO和LRU之间。
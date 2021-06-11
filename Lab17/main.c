#include <stdio.h>
#include <stdlib.h>
#define MAX_SIZE 100 

int memory[MAX_SIZE];   // 内存空间
int capacity;           // 内存空间容量
int size;               // 内存空间当前大小
//int queue[MAX_SIZE];    //元素为所在内存的位置，即，memory的下标
int head;               // FIFO中的队首指针
// int tail;
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

// int fifo1(int page){
//     if(find_page(page)){
//         printf("页%d在内存中找到，无需置换\n",page);
//         return 0;
//     }
//     else{
//         if(size < capacity){
//             printf("内存未满，页%d被插入到内存\n",page);
//             memory[size] = page;
//             queue[tail] = size;
//             size++;
//             tail = (tail + 1) % MAX_SIZE;
//         }
//         else{
//             printf("内存已满，页%d被换出，页%d被换入\n",memory[queue[head]],page);
//             memory[queue[head]] = page;
//             queue[tail] = queue[head];
//             head = (head + 1) % MAX_SIZE;
//             tail = (tail + 1) % MAX_SIZE;
//         }
//         return 1;
//     } 
// }

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

// 基于矩阵的LRU策略
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

int main(){
    int len,faultNum,i = 0,j;
    printf("请输入页内存大小（页数）\n");
    scanf("%d",&capacity);
    printf("请输入测试序列长度：");
    scanf("%d",&len);
    printf("请输入测试序列\n");
    for(int i = 0;i < len;i++){
        scanf("%d",&testCase[i]);
    }
    faultNum = 0;
    for(int i = 0;i < len;i++){
        faultNum += fifo(testCase[i]);
    }
    printf("fifo:缺页次数为：%d\n",faultNum);

    printf("-----------------------------------\n");
    faultNum = 0;
    size = 0;
    for(int i = 0;i < len;i++){
        faultNum += LRU_stack(testCase[i]);
    }
    printf("LRU based on stack:缺页次数为：%d\n",faultNum);

    printf("-----------------------------------\n");
    faultNum = 0;
    size = 0;
    for(int i = 0;i < len;i++){
        faultNum += LRU_matrix(testCase[i]);
    }
    printf("LRU based on matrix:缺页次数为：%d\n",faultNum);

    printf("-----------------------------------\n");
    faultNum = 0;
    size = 0;
    for(int i = 0;i < len;i++){
        faultNum += clock(testCase[i]);
    }
    printf("CLOCK:缺页次数为：%d\n",faultNum);
    return 0;
}
#define TEXT_SIZE 4*1024  /* = PAGE_SIZE, size of each message */
#define TEXT_NUM 1      /* maximal number of mesages */
    /* total size can not exceed current shmmax,
       or an 'invalid argument' error occurs when shmget */
#define MAX 1024
/* a demo structure, modified as needed */
struct shared_struct {
    int flag; /* flag = 0: buffer writable; others: readable */
    char mtext[TEXT_SIZE]; /* buffer for message reading and writing */
};

#define PERM S_IRUSR|S_IWUSR|IPC_CREAT

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)

/* 用于计算进入共享内存的写线程数目，若大于1则证明出错 */
static int counter = 0; 
/* 用于计算已完成当前信息读取的读线程数目，以便确定何时写线程可以进入共享内存 */
static int read_cnt = 0; 
  /* number of process(s) in the critical section */
int level[MAX];
  /* level number of processes 0 .. MAX_N-1 */
int waiting[MAX-1];
  /* waiting process of each level number 0 .. MAX_N-2 */
//int max_num = 20; /* default max thread number */
int writer_num; /* 写线程总数目 */
int reader_num; /* 读线程总数目 */
pthread_mutex_t read_cnt_lock; /* 用于read_cnt的互斥锁 */
pthread_mutex_t flag_lock; /* 用于flag的互斥锁 */
key_t key; /* 用于共享内存的键值 */


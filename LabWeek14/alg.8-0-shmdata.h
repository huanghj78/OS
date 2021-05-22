#define TEXT_SIZE 4*1024  /* = PAGE_SIZE, size of each message */
#define TEXT_NUM 1      /* maximal number of mesages */
    /* total size can not exceed current shmmax,
       or an 'invalid argument' error occurs when shmget */
#define MAX 1024
/* a demo structure, modified as needed */
struct shared_struct {
    int written; /* flag = 0: buffer writable; others: readable */
    char mtext[TEXT_SIZE]; /* buffer for message reading and writing */
};

#define PERM S_IRUSR|S_IWUSR|IPC_CREAT

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)



#define MAX_N 1024

static int counter = 0;
static int read_cnt = 0;
  /* number of process(s) in the critical section */
int level[MAX_N];
  /* level number of processes 0 .. MAX_N-1 */
int waiting[MAX_N-1];
  /* waiting process of each level number 0 .. MAX_N-2 */
//int max_num = 20; /* default max thread number */
int writer_num;
int reader_num;
pthread_mutex_t read_cnt_lock;
pthread_mutex_t written_lock;
key_t key;


#define TEXT_SIZE 4 * 1024 /* = PAGE_SIZE, size of each message */
#define TEXT_NUM 3        /* maximal number of mesages */
                           /* total size can not exceed current shmmax,
                              or an 'invalid argument' error occurs when shmget */

//学生结构类型，存放学生的学号和姓名
typedef struct{
  char id[50];
  char name[50];
}student;

/* a demo structure, modified as needed */
struct shared_struct
{
  int written; /* flag = 0: buffer writable; others: readable */
  int in;//队列头指针
  int out;//队列尾指针
  int end;//进程结束标志位
  student stu[TEXT_NUM + 1]; /* buffer for message reading and writing */
};



#define PERM S_IRUSR | S_IWUSR | IPC_CREAT

#define ERR_EXIT(m)     \
  do                    \
  {                     \
    perror(m);          \
    exit(EXIT_FAILURE); \
  } while (0)

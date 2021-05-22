#define TEXT_SIZE 50  // maximal length of the name
#define TEXT_NUM 5    /* maximal number of mesages */
                      /* total size can not exceed current shmmax,
                         or an 'invalid argument' error occurs when shmget */

//学生结构类型，存放学生的学号和姓名
typedef struct {
  int flag;  // 1表示被删除，0表示未被删除
  int id;
  char name[TEXT_SIZE];
} student;

/* a demo structure, modified as needed */
struct shared_struct {
  int lock;  // 1表示共享内存已被占用，0表示未被占用
  student stu[TEXT_NUM + 1];  //学生信息的静态数组
};

#define PERM S_IRUSR | S_IWUSR | IPC_CREAT

#define ERR_EXIT(m)     \
  do {                  \
    perror(m);          \
    exit(EXIT_FAILURE); \
  } while (0)

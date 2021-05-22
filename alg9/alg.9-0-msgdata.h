#define TEXT_SIZE 512

/* message structure */
struct msg_struct {
    long int msg_type;
    char mtext[TEXT_SIZE]; /* binary data */
};

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)



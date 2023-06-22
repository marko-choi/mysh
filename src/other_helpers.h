#ifndef __OTHER_HELPERS_H__
#define __OTHER_HELPERS_H__

// .h files contain functions and macros
// Network Support - Server Side
#ifndef SERVER_PORT
    #define SERVER_PORT 30000
#endif

#ifndef MAX_CONNECTIONS
    #define MAX_CONNECTIONS 1000
#endif

#ifndef MAX_BACKLOG
    #define MAX_BACKLOG 5
#endif

#ifndef MAX_NAME
    #define MAX_NAME 10
#endif

#ifndef MAX_USER_MSG
    #define MAX_USER_MSG 128
#endif

/*
 * Under our chat protocol, the maximum size of a message sent by
 * a server is MAX(username) + space + MAX(user message) + CRLF
 */
#ifndef MAX_PROTO_MSG
    #define MAX_PROTO_MSG MAX_NAME+1+MAX_USER_MSG+2
#endif

/* Working with string functions to parse/manipulate the contents of
 * the buffer can be convenient. Since we are using a text-based
 * protocol (i.e., message contents will consist only of valid ASCII
 * characters) let's leave 1 extra byte to add a NULL terminator
 * so that we can more easily use string functions. We will never
 * actually send a NULL terminator over the socket though.
 */
#ifndef BUF_SIZE
    #define BUF_SIZE MAX_PROTO_MSG+1 
#endif


typedef struct ps_lst {
    char cmd[MAX_STR_LEN];
    char cmd_lst[MAX_STR_LEN * 2];
    pid_t pid;
    int num;
    int status; // 0 = running, 1 = aborted / ended;
    struct ps_lst *next_process; 
} *ps_lst;

struct listen_sock { // server sock for parent (to listen to port)
    struct sockaddr_in *addr;
    int sock_fd;
};

struct client_sock {
    int sock_fd;
    int state;
    char *username;
    char buf[BUF_SIZE];
    int inbuf;
    struct client_sock *next;
};

struct server_sock { // server_sock for child (clients) 
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};


ps_lst process_lst;

// Signal handlers
void empty_handler(int sig);
void mysh_handler(int sig);
void bg_process_done(int sig);

// Expands all variables
void expand_variables(char **token_arr);

// Executes commands 
void command_execution(int *token_count, char **token_arr);


/* Recurses files given the path and depth
 */
void recurse_file(char *path, int depth, int current_depth, bool f_flag, char *f_substring);

/*
    Helper for pipe function (Creates child processes of mysh and runs the builtins)
    - Chains different lengths of pipes together
*/
void pipe_helper(char **commands, int pipe_num);

int setup_server_socket(char *port);

/*
 * Initialize a server address associated with the required port.
 * Create and setup a socket for a server to listen on.
 */
int setup_server_socket_helper(struct listen_sock *s, char *port);

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n);

/* 
 * Reads from socket sock_fd into buffer *buf containing *inbuf bytes
 * of data. Updates *inbuf after reading from socket.
 *
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_socket(int sock_fd, char *buf, int *inbuf);

/*
 * Search src for a network newline, and copy complete message
 * into a newly-allocated NULL-terminated string **dst.
 * Remove the complete message from the *src buffer by moving
 * the remaining content of the buffer to the front.
 *
 * Return 0 on success, 1 on error.
 */
int get_message(char **dst, char *src, int *inbuf);

/*
 * Write a string to a socket.
 *
 * Return 0 on success.
 * Return 1 on error.
 * Return 2 on disconnect.
 * 
 * See Robert Love Linux System Programming 2e p. 37 for relevant details
 */
int write_to_socket(int sock_fd, char *buf, int len);

/* 
 * Send a string to a client.
 * 
 * Input buffer must contain a NULL-terminated string. The NULL
 * terminator is replaced with a network-newline (CRLF) before
 * being sent to the client.
 * 
 * On success, return 0.
 * On error, return 1.
 * On client disconnect, return 2.
 */
int write_buf_to_client(struct client_sock *c, char *buf, int len);

/* 
 * Remove client from list. Return 0 on success, 1 on failure.
 * Update curr pointer to the new node at the index of the removed node.
 * Update clients pointer if head node was removed.
 */
int remove_client(struct client_sock **curr, struct client_sock **clients);

/* 
 * Read incoming bytes from client.
 * 
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if client socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_client(struct client_sock *curr);

int create_client(char **token_arr);
int create_server(char **token_arr, struct listen_sock s);

void sigint_handler(int code);
int server_accept_connection(int fd, struct client_sock **clients);
void server_clean_exit(struct listen_sock s, struct client_sock *clients, int exit_status);

int get_process_num();
void increment_process_num();
void decrement_process_num();

struct listen_sock* get_server_socket();
struct client_sock* get_client_sockets();

#endif 
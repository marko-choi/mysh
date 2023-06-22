#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <arpa/inet.h>     /* inet_ntoa */
#include <netdb.h>         /* gethostname */
#include <netinet/in.h>    /* struct sockaddr_in */
#include <assert.h>
#include <ctype.h>

#include "io_helpers.h"
#include "other_helpers.h"
#include "builtins.h"
#include "variables.h"


int process_num = 1;
struct listen_sock s; // Server socket
struct client_sock *clients; // Linked list of clients


void empty_handler(int sig) {}
void mysh_handler(int sig) {
    display_message("\nmysh$ ");
}

void bg_process_done(int sig) {
    
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);

    if (pid > 0) {

        int child_num = 0;
        char child_cmd[MAX_STR_LEN * 2];

        ps_lst curr = process_lst;
        while (curr != NULL){
            if (curr->pid == pid) {
                child_num = curr->num;
                curr->status = 1;

                child_cmd[0] = '\0';
                strncpy(child_cmd, curr->cmd_lst, MAX_STR_LEN * 2 - 1);
    
            }
            curr = curr->next_process;
        }

        char done_msg[MAX_STR_LEN * 3];
        done_msg[0] = '\0';
        sprintf(done_msg, "[%d]+  Done \t %s\n", child_num, child_cmd);

        display_message(done_msg);

        // Reset counter once all processes are done
        if (child_num + 1 == process_num) {
            process_num = 1;
        }

    }
}
// Strings and heaps 
void expand_variables(char **token_arr) {
    int index = 0;
    while (token_arr[index] != NULL){                
        if (strncmp(token_arr[index], "$", 1) == 0){
            // replaces pointer of <token_arr[i]> to pointer of the <variable> value                    
            char var_name[strlen(token_arr[index])];
            strncpy(var_name, &token_arr[index][1], strlen(token_arr[index]));

            char* value = value_lookup(var_name);
            if (value != NULL){
                token_arr[index] = value;
            }
        }
        index += 1;
    }
}

// Commands
void command_execution(int *token_count, char **token_arr){
    if (*token_count >= 1) {
        bn_ptr builtin_fn = check_builtin(token_arr[0]);
        if (builtin_fn != NULL) {

            ssize_t err = builtin_fn(token_arr);
            if (err == -1) {
                display_error("ERROR: Builtin failed: ", token_arr[0]);
            }
        }
        // Assignment statement
        else if (strchr(token_arr[0], '=') != NULL ) {
            char *curr_ptr = (char*) strtok(token_arr[0], "=");
            char *name = curr_ptr;
            curr_ptr= (char*) strtok(NULL, DELIMITERS);
            
            // Empty string assignment handling
            if (curr_ptr == NULL) {
                char *value = "\0";
                assign_value(name, value); 
            } else {
                char *value = curr_ptr;
                assign_value(name, value); 
            }
        } 
        else { 
            
            int value;
            pid_t ret = fork();
            if (ret == -1){
                display_error("ERROR: Fork failed", "");
            }
            else if (ret == 0) {
                execvp(token_arr[0], token_arr);
                display_error("ERROR: Unrecognized command: ", token_arr[0]);
                exit(1);
            }
            wait(&value);

        }
    }
}

// File System
void recurse_file(char *path, int depth, int current_depth, bool f_flag, char* f_substring){
    if (current_depth == 0) current_depth = 1;
    struct dirent *diret;
    DIR *dir = opendir(path);

    while ((diret = readdir(dir)) != NULL && depth >= current_depth) {
            char file[500];
            char print_file[500];
            print_file[0] = '\0';
            file[0] = '\0';
            if (f_substring == NULL) {
                sprintf(file, "%s", diret->d_name);
                
                for (int i = 1; i < current_depth; i++){
                    strcat(print_file, " ");
                }
                strcat(print_file, file);
                strcat(print_file, "\n");
                display_message(print_file);
           
            }
            else if (strstr(diret->d_name, f_substring) != NULL){
                sprintf(file, "%s", diret->d_name);

                for (int i = 1; i < current_depth; i++){
                    strcat(print_file, " ");
                }
                strcat(print_file, file);
                strcat(print_file, "\n");
                display_message(print_file);
            }
            
            if (diret->d_type == DT_DIR && strcmp(diret->d_name, ".") != 0 && strcmp(diret->d_name, "..") != 0){
               
                strcpy(file, path);
                strcat(file, "/");
                strcat(file, diret->d_name);

                recurse_file(file, depth, current_depth+1, f_flag, f_substring);
            }
    }

    int err = closedir(dir);
    if (err == -1){
        display_error("ERROR: Cannot close directory", "");
    }
}

// Pipes
void pipe_helper(char **commands, int pipe_num) {
    //commands = {"cmd1 arg1 arg2", "cmd2 arg1 arg2"}
    int value;
    int current_file;
    int pipe_fd[2];

    for (int i = 0; i <= pipe_num; i++){
        if (pipe(pipe_fd) == -1) {
            display_error("ERROR: Pipe cannot be executed.", "");
            exit(1);
        }

        pid_t result = fork();

        if (result < 0){ display_error("ERROR: Fork command failed.", ""); exit(1); }
        else if (result == 0) {
            
            if (i != 0) { dup2(current_file, STDIN_FILENO);} // Reroutes current command's standard input to pipe_fd[0] (pipe reading end)
            if (i != pipe_num) { dup2(pipe_fd[1], STDOUT_FILENO); } // Reroutes last command's pipe_fd[1] (pipe writing end) to standard output 

            // execute current comand
            char *cmd[MAX_STR_LEN] = {NULL};
            int cmd_count = tokenize_input(commands[i], cmd);
            
            //reassignment statements
            expand_variables(cmd);
            command_execution(&cmd_count, cmd);

            
            if(close(pipe_fd[0]) == -1) { display_error("ERROR: Cannot close child reading end", "");}
            if(close(pipe_fd[1]) == -1) { display_error("ERROR: Cannot close child writing end", "");}
            exit(1);
        } else {
            // Parent process 
            wait(&value);
            current_file = pipe_fd[0]; // stores reading end of current file
            if (close(pipe_fd[1]) == -1) display_error("ERROR: Cannot close writing end of child pipe", "");
        }

        
    }
}

// Network Support (helpers)
int setup_server_socket(char *port){
    if (setup_server_socket_helper(&s, port) != 0) {
        return -1;
    }
    return 0;

}

int setup_server_socket_helper(struct listen_sock *s, char *port) {
    
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        // display_error("ERROR: Server address failed to store on heap", "");
        return -1;
    }
    s->addr->sin_family = AF_INET; // Allow sockets across machines.
    s->addr->sin_port = htons(strtol(port, NULL, 10)); // The port the process will listen on.
    memset(&(s->addr->sin_zero), 0, 8); // Clear this field; sin_zero is used for padding for the struct.
    s->addr->sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces.
    

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        display_error("ERROR: Server socket failed", "");
        // perror("server socket");
        return -1;
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        display_error("ERROR: Socket reusability option failed", "");
        // perror("setsockopt");
        return -1;
    }

    // Bind the selected port to the socket.
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
        display_error("ERROR: Server bind set up failed", "");
        close(s->sock_fd);
        return -1;
    }

    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        display_error("ERROR: Server listen set up failed.", "");
        close(s->sock_fd);
        return -1;
    }

    return 0;
}

int find_network_newline(const char *buf, int inbuf) {
    for (int i = 0; i < inbuf - 1; i++){
        if (buf[i] == '\r' && buf[i + 1] == '\n'){
            return i + 2;
        }
    }
    return -1;
}

int read_from_socket(int sock_fd, char *buf, int *inbuf) {
    int num_read = read(sock_fd, buf + *inbuf, BUF_SIZE - *inbuf);

    if (num_read == 0) return 1;  // occurs when client disconnects from server
    else if (num_read == -1) return -1; 

    *inbuf += num_read;
    for (int i = 0; i <= *inbuf-2; i++){
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return 0;
        }
    }

    return 2;
}

int get_message(char **dst, char *src, int *inbuf) {
    
    int msg_len = find_network_newline(src, *inbuf);
    if (msg_len == -1) return 1;

    *dst = malloc(BUF_SIZE);
    if (*dst == NULL) {
        display_error("ERROR: Failed to allocate memory on the heap for message", "");
        return 1;
    }
    
    memmove(*dst, src, msg_len - 2);
    (*dst)[msg_len-2] = '\0';

    // increases the size of the msg_len by src + msg_len
    memmove(src, src + msg_len, BUF_SIZE - msg_len);

    *inbuf -= msg_len;
    return 0;
}

int write_to_socket(int sock_fd, char *buf, int len) {
    int ret;
    while (len != 0 && (ret = write(sock_fd, buf, len))){
        if (ret == -1){
            if (errno == EINTR) continue;
            if (errno == EPIPE) return 2;
            break;
        }

        len -= ret;
        buf += ret;
    }

    if (len != 0) return 1;
    return 0;
}


// Network Support - Server side
int sigint_received = 0;

void sigint_handler(int code) {
    sigint_received = 1;
}

/*
 * Wait for and accept a new connection.
 * Return -1 if the accept call failed.
 */
int server_accept_connection(int fd, struct client_sock **clients) {
    // fprintf(stderr, "Accepting new server connection");
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int num_clients = 0;
    struct client_sock *curr = *clients;
    while (curr != NULL && num_clients < MAX_CONNECTIONS && curr->next != NULL) {
        curr = curr->next;
        num_clients++;
    }

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);
    
    if (client_fd < 0) {
        display_error("ERROR: Server failed to accept newly established connection", "");
        // perror("server: accept");
        close(fd);
        return -1;
    }

    if (num_clients == MAX_CONNECTIONS) {
        close(client_fd);
        return -1;
    }

    struct client_sock *newclient = malloc(sizeof(struct client_sock));
    newclient->sock_fd = client_fd;
    newclient->inbuf = newclient->state = 0;
    newclient->username = NULL;
    newclient->next = NULL;
    memset(newclient->buf, 0, BUF_SIZE);
    if (*clients == NULL) {
        *clients = newclient;
    }
    else {
        curr->next = newclient;
    }

    return client_fd;
}

/*
 * Close all sockets, free memory, and exit with specified exit status.
 */

 void stop_server_handler(int sig) {

 }

void server_clean_exit(struct listen_sock s, struct client_sock *clients_ll, int exit_status) {
    struct client_sock *tmp;
    while (clients_ll) {
        tmp = clients_ll;
        close(tmp->sock_fd);
        clients_ll = clients_ll->next;
        free(tmp->username);
        free(tmp);
    }
    // close(s.sock_fd);
    free(s.addr);
    // fprintf(stderr, "Killed server\n");
}

int create_server(char **token_arr, struct listen_sock s) {
    // This line causes stdout not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    
    /*
     * Turn off SIGPIPE: write() to a socket that is closed on the other
     * end will return -1 with errno set to EPIPE, instead of generating
     * a SIGPIPE signal that terminates the process.
     */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        display_error("ERROR: Signal did not set to disable socket write end", "");
        return -1;
    }

    // Linked list of clients
    clients = NULL;
    
    // Set up SIGINT handler
    struct sigaction sa_sigint;
    memset (&sa_sigint, 0, sizeof (sa_sigint));
    sa_sigint.sa_handler = sigint_handler;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);

    signal(SIGUSR1, stop_server_handler);
    
    int exit_status = 0;
    
    int max_fd = s.sock_fd;

    fd_set all_fds, listen_fds;
    
    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);

    do {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (sigint_received) break;
        if (nready == -1) {
            if (errno == EINTR) continue;
            // perror("server: select");
            display_error("ERROR: Server failed to select", "");
            return -1;
        }

        /* 
         * If a new client is connecting, create new
         * client_sock struct and add to clients linked list.
         */
        if (FD_ISSET(s.sock_fd, &listen_fds)) {
            int client_fd = server_accept_connection(s.sock_fd, &clients);

            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
        }

        if (sigint_received) break;

        /*
         * Accept incoming messages from clients,
         * and send to all other connected clients.
         */
        struct client_sock *curr = clients;
        while (curr) {
            if (!FD_ISSET(curr->sock_fd, &listen_fds)) {
                curr = curr->next;
                continue;
            }
            // display_message("Before reading client\n");
            int client_closed = read_from_client(curr);
            // display_message("After reading client\n");
            // If error encountered when receiving data
            if (client_closed == -1) {
                client_closed = 1; // Disconnect the client
            }
                
            char *msg;
            // Loop through buffer to get complete message(s)
            while (client_closed == 0 && !get_message(&msg, curr->buf, &(curr->inbuf))) {

                char line[BUF_SIZE];
                line[0] = '\0';

                strncat(line, msg, MAX_USER_MSG);
                free(msg);
                int data_len = strlen(line);
                
                struct client_sock *dest_c = clients;
                
                char test[BUF_SIZE+2];
                test[0] = '\0';
                sprintf(test, "\n%s", line);
                display_message(test);
                display_message("mysh$ ");
                
                while (dest_c) {

                    if (dest_c != curr) {
                        int ret = write_buf_to_client(dest_c, line, data_len);

                        if (ret != 0) {
                            if (ret == 2) {
                               close(dest_c->sock_fd);
                                FD_CLR(dest_c->sock_fd, &all_fds);
                                assert(remove_client(&dest_c, &clients) == 0); // If this fails we have a bug
                                continue;
                            }
                        }
                    }
                    dest_c = dest_c->next;
                }
            }

            // printf(stderr, "!  Done sending messages\n");            
            if (client_closed == 1) { // Client disconnected
                // Note: Never reduces max_fd when client disconnects
                FD_CLR(curr->sock_fd, &all_fds);
                close(curr->sock_fd);
                assert(remove_client(&curr, &clients) == 0); // If this fails we have a bug
            }
            else {
                curr = curr->next;
            }
        }
    } while(!sigint_received);
    
    server_clean_exit(s, clients, exit_status);
    return exit_status;
}

// Network Support - Client Side 

int client_sigint_received = 0;

void client_sigint_handler(int code) {
    client_sigint_received = 1;
}

int create_client(char **token_arr) {
    /**
     * char *token_arr[MAX_BUF] constituents
     * 0 - "start-client"
     * 1 - port number
     * 2 - host name
     */
    struct server_sock s;
    s.inbuf = 0;

    // Set up SIGINT handler
    struct sigaction sa_sigint;
    memset (&sa_sigint, 0, sizeof (sa_sigint));
    sa_sigint.sa_handler = client_sigint_handler;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);
    
    
    // Create the socket FD.
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0) {
        display_error("ERROR: Failed to set up client socket", "");
        return -1;
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(strtol(token_arr[1], NULL, 10));

    if (inet_pton(AF_INET, token_arr[2], &server.sin_addr) < 1) {
        display_error("ERROR: Client internet address is wrong.", "");
        close(s.sock_fd);
        return -1;
    }

    // Connect to the server.
    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        display_error("ERROR: Client cannot connect to server.", "");
        close(s.sock_fd);
        return -1;
    }

    // // Retrieves hostname from token_arr input
    // char hostname[MAX_STR_LEN + 2];
    // hostname[0] = '\0';
    // int hostname_length = strlen(token_arr[2]);    
    // strncat(hostname, token_arr[2], hostname_length);

    // hostname[hostname_length] = '\r';
    // hostname[hostname_length+1] = '\n';
    // hostname[hostname_length+2] = '\0';

    // if (write_to_socket(s.sock_fd, hostname, hostname_length+2)) {
    //     display_error("ERROR: cannot send username.", "");
    //     return 1;
    // }

    /*
     * Step 1: Prepare to read from stdin as well as the socket,
     * by setting up a file descriptor set and allocating a buffer
     * to read into. It is suggested that you use buf for saving data
     * read from stdin, and s.buf for saving data read from the socket.
     * Why? Because note that the maximum size of a user-sent message
     * is MAX_USR_MSG + 2, whereas the maximum size of a server-sent
     * message is MAX_NAME + 1 + MAX_USER_MSG + 2. Refer to the macros
     * defined in socket.h.
     */
    
    // Setting up file descriptor set
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(s.sock_fd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    int max_fd = s.sock_fd;
    // allocating buffer to read into

    // buf - saves data from stdin
    // s.buf - saves data read from the socket
    strcpy(s.buf, "\0");
    
    /*
     * Step 2: Using select, monitor the socket for incoming mesages
     * from the server and stdin for data typed in by the user.
     */

    while(!client_sigint_received) {
        // Monitors sever and stdin for data typed by the user
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(s.sock_fd, &read_fds);

        
        int num_ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (num_ready < 0) {  
            return 1;
            }
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)){

                char buffer[MAX_PROTO_MSG];
                buffer[0] = '\0';
                fgets(buffer, MAX_STR_LEN, stdin);

                int read_len = strlen(buffer);
                if (read_len == 0) {break; } // STDIN CLOSES
                buffer[read_len] = '\r';
                buffer[read_len + 1] = '\n';
                
                write(s.sock_fd, &buffer, read_len + 2);
        }

        /*
         * Step 4: Read server-sent messages from the socket.
         * The read_from_socket() and get_message() helper functions
         * will be useful here. This will look similar to the
         * server-side code.
         */
        if (FD_ISSET(s.sock_fd, &read_fds)){
            // fprintf(stderr, "!  Stuck in server\n");
            int server_message = read_from_socket((&s)->sock_fd, (&s)->buf, &((&s)->inbuf));
            if (server_message != 0) { break; }

            char *msg;
            get_message(&msg,s.buf, &(s.inbuf));

            fflush(stdout);
            printf("%s", msg);
            free(msg);
        }
    }
    close(s.sock_fd);
    return 0;
}

// Network Support - Client Side (helpers)
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
int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    // Replace network-newline (CR-LF) with NULL terminator
    buf[len] = '\r';
    buf[len+1] = '\n';
    buf[len+2] = '\0';
    
    // Write to socket call:
    //      c->sock_fd  (socket file descriptor)
    //      buf         buffer
    //      len         length of the buf
    return write_to_socket(c->sock_fd, buf, len + 2);
}

/* 
 * Remove client from list. Return 0 on success, 1 on failure.
 * Update curr pointer to the new node at the index of the removed node.
 * Update clients pointer if head node was removed.
 */
int remove_client(struct client_sock **curr, struct client_sock **clients) {

    
    // Case: head node == curr
    if ((*clients) == (*curr)) {

        struct client_sock *pointer = *clients;    
        close((*clients)->sock_fd);
        if ((*clients)->next == NULL) {
            *clients = NULL;
            *curr = NULL;
            free(pointer);
        } else {
            *clients = (*clients)->next;
            *curr = *clients;
            free(pointer);
        }
        return 0;
    }

    // Case: curr is not the head node 
    while ((*clients)->next != NULL) {

        if ((*clients)->next == *curr) {
                        
            close((*clients)->next->sock_fd);        
            struct client_sock *pointer = (*clients)->next;

            (*clients)->next = (*clients)->next->next;
             *curr = (*clients)->next; 
            free(pointer);

           
            
            return 0;

        }
        (*clients) = (*clients)->next;
    }

    return 1; // Couldn't find the client in the list, or empty list
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

// ================================================================
// 
//                              GETTERS 
//
// ================================================================

int get_process_num() { return process_num; }
void increment_process_num(){ process_num++; }
void decrement_process_num(){ process_num--; }

struct listen_sock *get_server_socket(){ 
    return &s; 
    }
struct client_sock *get_client_sockets(){ return clients; } // Linked list of clients

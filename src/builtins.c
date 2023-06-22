#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "other_helpers.h"


int server_status = 0;
int server_pid; 

// ====== Command execution =====

/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;

    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }

    return BUILTINS_FN[cmd_num];
}


// ===== Builtins =====

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_echo(char **tokens) {
    ssize_t index = 1;

    if (tokens[index] != NULL) {
        display_message(tokens[index]);
        index += 1;
    }
    while (tokens[index] != NULL) {
        display_message(" ");
        display_message(tokens[index]);        
        index += 1;
    }
    display_message("\n");

    return 0;
}

ssize_t bn_ls(char **tokens){
    // utilizes opendir
    int index = 1;
    int depth = 0;
    struct dirent *diret;
    char *path = "";
    DIR *dir;
    bool f_flag = false, rec_flag = false, d_flag = false;
    bool normal_path = false;
    char *f_substring;
    f_substring = NULL;

    while(tokens[index] != NULL){
        // read --f substring
        if(strncmp(tokens[index], "--f", 4) == 0) {
            // Displays all directory contnents that contain the <substirng>
            if (tokens[index+1] == NULL || strstr(tokens[index+1], "--") != NULL){
                display_error("ERROR: --f requires a substring", "");
                return -1;
            }
            f_substring = tokens[index+1];
            f_flag = true;
            index += 2;
        }

        // read --rec [path]
        else if(strncmp(tokens[index], "--rec", 6) == 0) {
            rec_flag = true;
            if (tokens[index+1] == NULL || strstr(tokens[index+1], "--") != NULL){
                path = ".";
                index += 1;
            } else {
                path = tokens[index+1];
                index += 2;
            }
        }        

        // read --d <depth value>
        else if(strncmp(tokens[index], "--d", 4) == 0) {
            char *ptr;

            if(tokens[index+1] == NULL || strstr(tokens[index+1], "--") != NULL){
                display_error("ERROR: --d requires a depth value", "");
                return -1;
            }
            depth = strtol(tokens[index + 1], &ptr, 10);
            d_flag = true;
            index += 2;
        }
        
        else {
            normal_path = true;
            dir = opendir(tokens[index]);
            index += 1;
        }
    }

    if (tokens[1] == NULL){
        dir = opendir(".");
    }

    // Error handling 
    if (dir == NULL && strcmp(path, "") == 0) {
        display_error("ERROR: Invalid path", "");
        return -1;}
    
    if ((d_flag && !rec_flag) || (!d_flag && rec_flag)) { 
        display_error("ERROR: Must provide both --d and --rec flags", ""); 
        return -1;}

     if (rec_flag && normal_path) { 
        display_error("ERROR: Too many paths provided", "");
        return -1;}
    // Error handling for <depth>
    if (depth < 0) {
        display_error("ERROR: Negative depth provided", "");
        return -1;
    }
    if (depth > 0) {
        recurse_file(path, depth, 0, f_flag, f_substring);
    }

    // Directory looping
    else if (f_flag){
        // loops through all of the tokens     
        while ((diret = readdir(dir)) != NULL) {
            if (strstr(diret->d_name, f_substring) != NULL){
                char file[500];
                sprintf(file, "%s\n", diret->d_name );
                display_message(file);
            }
        }
        int err = closedir(dir);
        if (err == -1){
            display_error("ERROR: Directory not closed.", "");
        }        
    } else {
        while ((diret = readdir(dir)) != NULL) {
            char file[500];
            sprintf(file, "%s\n", diret->d_name );
            display_message(file);
        }   

        int err = closedir(dir);
        if (err == -1){
            display_error("ERROR: Directory not closed.", "");
        }
    }

    return 0;
}

ssize_t bn_cd(char **tokens){
    // Checks if the correct amount of arguments are passed through
    int index = 1;
    while (tokens[index] != NULL){
        if (index > 1) {
            display_error("ERROR: Too many arguments provided", "");
            return -1;
        }
        index++;
    }

    if (tokens[1] != NULL) {
        int count_dots = 0;
        size_t cd_length = strlen(tokens[1]);
        for (int i=0; i < cd_length; i++){
            if(strncmp(&(tokens[1][i]), ".", 1) != 0) { break; }
            count_dots++;
        }
    
        // Expands the '...' into '../.. etc'
        char path[MAX_STR_LEN * 3];
        path[0] = '\0';
        if (count_dots > 2) {
            for(int i=2; i<count_dots; i++){
                strcat(path, "../");
            }
            strcat(path, "..");
        }

        if (strcmp(path, "") != 0){
            // Checks if the directory changed is correct or not
            if (chdir(path) == -1){
                display_error("ERROR: Invalid path", "");
                return -1;
            }
        } else {
            if (chdir(tokens[1]) == -1){
                display_error("ERROR: Invalid path", "");
                return -1;
            }
            
        }

    } else {
        if (chdir(tokens[1]) == -1){
                display_error("ERROR: Invalid path", "");
                return -1;
            }
    }
    return 0;
}

ssize_t bn_cat(char **tokens){

    char line[MAX_STR_LEN + 1];
    FILE* file;

    // Checks if there is an input source
    if (!tokens[1]){
        
        // Checks for stdin
        if(isatty(0)){
            display_error("ERROR: No input source provided", "");
            return -1;
        }
        
        int val = -1;
        val = getchar();
        if (val == EOF || val == -1){
            display_error("ERROR: No input source provided", "");
            return -1;
        }
        ungetc(val, stdin);
        

         while (fgets(line, MAX_STR_LEN + 1, stdin) != NULL) {
            // fails to read == NULL
            char curr_line[500];
            sprintf(curr_line, "%s", line);
            display_message(curr_line);
        }
    
    } else {

        // Checks if the file can be opened
        file = fopen(tokens[1], "r");
        if (file == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }

        // Loops through with fgets to read every line in the file
        while (fgets(line, MAX_STR_LEN + 1, file) != NULL) {
            // fails to read == NULL
            char curr_line[500];
            sprintf(curr_line, "%s", line);
            display_message(curr_line);
        }
        
        // Closes the file 
        if (fclose(file) != 0){
            display_error("ERROR: Cannot close file", "");
            return -1;
        }
    }

    return 0;
}

ssize_t bn_wc(char **tokens){

    FILE* file;
    bool has_file;
    char c, last_c;
    int word_count = 0, char_count = 0, new_line = 0;
     // Checks if there is an input source
    if (!tokens[1]){

        if(isatty(0)){
            
            // fprintf(stderr, "ERROR: No input source provided\n");
            display_error("ERROR: No input source provided", "");
            return -1;
        }

        // Checks for stdin
        int val = getchar();
        if (val == EOF){
            display_error("ERROR: No input source provided", "");
            return -1;
        }
        ungetc(val, stdin);

        while (1) {
            c = getchar() ; // reading the file
            if (c == EOF) {
                if (isgraph(last_c)){ word_count += 1; }
                break;
            } else if (isgraph(c) || isspace(c)) {
                // checks if it is a character
                char_count += 1;
                if (isspace(c) && isgraph(last_c)){ word_count += 1; }
                if (c == '\n') { new_line += 1; }
                last_c = c;
            }
        
        }
    } else {

        // Checks if the file can be opened
        file = fopen(tokens[1], "r");
        if (file == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }

        has_file = 1;

        // Counts the number of words, characters and newlines

        while (1) {
            c = fgetc(file) ; // reading the file
            if (c == EOF) {
                if (isgraph(last_c)){ word_count += 1; }
                break;
            } else if (isgraph(c) || isspace(c)) {
                // checks if it is a character
                char_count += 1;
                if (isspace(c) && isgraph(last_c)){ word_count += 1; }
                if (c == '\n') { new_line += 1; }
                last_c = c;
            }
        
        }
    }

    char wc[500];
    sprintf(wc, "word count %d\n", word_count);
    display_message(wc);

    char cc[500];
    sprintf(cc, "character count %d\n", char_count);
    display_message(cc);

    char nl[500];
    sprintf(nl, "newline count %d\n", new_line);
    display_message(nl);

    // Closes the file 
    if (has_file){
        int error = fclose(file);
        if (error != 0){
            display_error("ERROR: Cannot close file", "");
            return -1;
        }
    }
    return 0;
}

ssize_t bn_kill(char **tokens){

    // correct usage: kill [pid] [signum]
    for (int i = 0; i < 2; i++) {
        if (tokens[i] == NULL) {
            display_error("ERROR: Correct usage: kill [pid] [signum]", "");
            return -1;
        }
    }

        int pid = strtol(tokens[1], NULL, 10);
        // if signum not provided, send SIGTERM
        int external_process = kill(pid, 0);

        if (tokens[2] == NULL) {
            // printf("Killing process: %d", pid);
            bool has_process = false;
            ps_lst head = process_lst;
            if (head->pid == pid) has_process = true;
            while (head->next_process != NULL){
                head = head->next_process;
                if (head->pid == pid) has_process = true;
                // printf("Current pid: %d", head->pid);
            }


            if (has_process || external_process == 0){
                kill(pid, SIGTERM);
            } else {
                // if process id invalid, report 
                display_error("ERROR: The process does not exist", "");
                return -1;
            }

        } else {
            // sends signal number <signum> to the process id
            // tokens[1] - pid
            // tokens[2] - signum
            bool error = false;
            int signum = strtol(tokens[2], NULL, 10);

            bool has_process = false;
            ps_lst head = process_lst;
            if (head->pid == pid) has_process = true;
            while (head->next_process != NULL){
                head = head->next_process;
                if (head->pid == pid) has_process = true;
            }

            if (has_process || external_process == 0 ){
                kill(pid, signum);
            } else {
                // if process id invalid, report 
                display_error("ERROR: The process does not exist", "");
                error = true;
            }
        

            // If the signal is invalid, report
            if (errno == EINVAL){
                display_error("ERROR: Invalid signal specified", "");
                error = true;
            }

            if (error) {
                return -1;
            }
        }


    return 0;
}

ssize_t bn_ps(char **tokens){

    // list process names and ids for all processes launched by the shell
    // need to store all the processes in a list

    // Sample calls
    // mysh$ sleep 50 &
    // [1] 8574
    // mysh$ sleep 20 &
    // [2] 8575
    // mysh$ ps
    // sleep 8575
    // sleep 8574
    // mysh$

    ps_lst head = process_lst;

    if (head->next_process != NULL){
        head = head->next_process;
        if (head->status == 0){
            char line[MAX_STR_LEN * 3];
            sprintf(line, "%s %d\n", head->cmd, head->pid);
            display_message(line);
        }
    }
    while (head->next_process != NULL){
        head = head->next_process;
        if (head->status == 0){
            char line[MAX_STR_LEN * 3];
            sprintf(line, "%s %d\n", head->cmd, head->pid);
            display_message(line);
        }
    }
    return 0;
}

ssize_t bn_start_server(char **tokens){
    
    if (tokens[1] == NULL) {
        display_error("ERROR: no port provided", "");
        return -1;
    }

    if (server_status != 0) {
        display_error("ERROR: server already running", "");
        return -1;
    }

    // Call start_server function in other_helpers.c
    if (setup_server_socket(tokens[1]) != 0) {
        return -1;
    } 
    struct listen_sock* server = get_server_socket();
    int ret = fork();
    
    if (ret == 0) {
        create_server(tokens, *server);
        exit(0);
    }

    // parent process continues on
    server_pid = ret;
    server_status = 1;

    return 0;
}

ssize_t bn_close_server(char **tokens){

    // calls close server function in other_helpers.c
    if (server_status == 0) {
        display_error("ERROR: No servers running", "");
        return -1;
    }                      
    
    signal(SIGCHLD, empty_handler);
    if (get_server_pid() != -1) { 
        kill(get_server_pid(), SIGINT);
    }

    signal(SIGCHLD, bg_process_done);
    return 0; 
}

ssize_t bn_send(char **tokens){
    
    int err = 0;

    if (tokens[1] == NULL) {
        err = 1; 
        display_error("ERROR: No port provided", "");
    }
    if (tokens[2] == NULL) {
        err = 1; 
        display_error("ERROR: No hostname provided", "");
    }
    if (tokens[3] == NULL) {
        err = 1; 
        display_error("ERROR: No message provided", "");
    }
    
    if (err != 0) return -1;

    // parse messsage here 
    int index = 3;
    
    char msg[MAX_STR_LEN * 10];
    msg[0] = '\0';

    while (tokens[index]) {
        strcat(msg, tokens[index]);
        if (tokens[index+1]) strcat(msg, " ");
        index++;
    }

    struct server_sock s;
    s.inbuf = 0;
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0) {       
        display_error("ERROR: Failed to set up client socket", "");
        return -1;
    }
   
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(strtol(tokens[1], NULL, 10));
    if (inet_pton(AF_INET, tokens[2], &server.sin_addr) < 1) {
        display_error("ERROR: Host name address is wrong","");
        close(s.sock_fd);
        return -1;
    }

    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        display_error("ERROR: Client cannot connect to server.", "");
        close(s.sock_fd);
        return -1;
    }
    strcat(msg, "\n\r\n");
    write(s.sock_fd, &msg, strlen(msg)+1);
    close(s.sock_fd);

    return 0;
}

ssize_t bn_start_client(char **tokens){
    int err = 0;
    if (tokens[1] == NULL) {
        err = 1;
        display_error("ERROR: No port provided", "");
    }
    if (tokens[2] == NULL) {
        err = 1; 
        display_error("ERROR: No hostname provided", "");
    }
    if (err != 0) return -1;

    // Error checking for the name
    int hostname_length = strlen(tokens[2]);
    if (hostname_length > MAX_STR_LEN) {
        display_error("ERROR: Host name %s too long.", tokens[2]);
        return -1;
    }


    create_client(tokens);
    return 0;
}

// ==============================================================
//                         Getter Functions 
// ==============================================================

int get_server_status() {
    return server_status;
}

int get_server_pid() {
    if (server_status != 0) {
        return server_pid;
    }
    return -1;
}
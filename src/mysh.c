#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "other_helpers.h"

void free_processes(struct ps_lst* process_lst);
void add_process(struct ps_lst* new_process, struct ps_lst* process_lst);
extern ps_lst process_lst;
// extern struct listen_sock s;
// extern struct client_sock *clients;

int main(int argc, char* argv[]) {

    // struct listen_sock s = get_server_socket();
    // struct client_sock *clients = get_client_sockets();

    char *prompt = "mysh$ ";

    char input_buf[MAX_STR_LEN + 1];
    char input_cp[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    input_cp[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};
    int token_count = 0;
    int ret = 0;

    // struct sigaction act;
    // act.sa_handler = handler;
    // sigset_t sigset;
    // sigaddset(&sigset, SIGINT);
    // sigprocmask(SIG_BLOCK, &sigset, NULL);

    signal(SIGCHLD, bg_process_done);
    signal(SIGINT, mysh_handler);
    
    // printf("sizeof ps_lst: %ld\n", sizeof(struct ps_lst));
    if (argc == 1) {
        process_lst = malloc(sizeof(struct ps_lst));
        process_lst->num = 0;
        process_lst->pid = getpid();
        process_lst->next_process = NULL;
        process_lst->status = 0;
    }


    while (1) {

        

        bool bg_process = false;
        // Part 1) Prompt and input tokenization
        // Display the prompt via the display_message function.
        if (argc == 1){

            // char prompt_line[MAX_STR_LEN];
            // sprintf(prompt_line, "mysh [%d]$ ", getpid());
            // display_message(prompt_line);
            display_message(prompt);

            ret = get_input(input_buf);
            strncpy(input_cp, input_buf, MAX_STR_LEN - 1);
            token_count = tokenize_input(input_buf, token_arr);
        }

        // Part 2) Expands variables (replaces all $var with their values)        
        expand_variables(token_arr);

        if (argc > 1) {

            for (int i = 1; i <= argc; i++){
                if (i != argc) token_arr[i - 1] = argv[i];
                if (i == argc) token_arr[i - 1] = NULL;
            }
            token_count = argc - 1;

        }

        // Part 3) Checks whether or not there is a & sign
        // If there is, then the command is run in the background processes
        
            // Starts the program as a background process
            // Displays [number] pid when the process is launched
            // (ie. [1] 25787 - similar behavior to bash)
            // Display [number]+ Done command when the process completes
                
                // Identical behavior to bash
                // We will only display Done messages
                // If a process terminates because of a signal, a Done is shown
                    // checks for background processing 
        int index = 0;
        // printf("%s %s %s %s %s\n", token_arr[0], token_arr[1], token_arr[2], token_arr[3], token_arr[4]);
        
        while (token_arr[index] != NULL){
            
            // char s[2] = "\0"; /* gives {\0, \0} */
            // s[0] = token_arr[index][strlen(token_arr[index]) - 1];
            // Checks for & symbol for the LAST argument
            // fprintf(stdout, "Result: %d\t%s\tString length: %ld\n", strcmp(token_arr[index], "&"), token_arr[index], strlen(token_arr[index]));
            if (strcmp(token_arr[index], "&") == 0){
                if (token_arr[index + 1] != NULL) {
                    display_error("ERROR: Must be & at the end of a command", "");
                } 
                else {
                    // display_message("   Initiating background processing ... \n");
                    // token_arr[index] = NULL;
                    bg_process = true;
                }
            }
            // else if (strcmp(s, "&") == 0) {
            //     if (token_arr[index + 1] != NULL) {
            //         display_error("ERROR: Must be & at the end of a command", "");
            //     } else {
            //         token_arr[index][strlen(token_arr[index]) - 1] = '\0';
            //         bg_process = true;
            //     }
            // }
    

            // Checks for & smybol as the last character of this argument

            index++;
        } 

        // char prompt_line[MAX_STR_LEN];
        // sprintf(prompt_line, "Background process: [%d] bg_process: %d\n", getpid(), bg_process);
        // display_message(prompt_line);
        
        // WIFEXITED(status);
        // pid_t child_pid = waitpid(-1, &status, WNOHANG);
        // if(child_pid != -1) {

        //     int child_num = 0;
        //     char child_cmd[MAX_STR_LEN];

        //     ps_lst curr = process_lst;
        //     while (curr != NULL){
        //         if (curr->pid == child_pid) {
        //             child_num = curr->num;
        //             strncpy(child_cmd, curr->cmd, MAX_STR_LEN - 1);
        //         }
        //         curr = curr->next_process;
        //     }
        //     char done_msg[MAX_STR_LEN * 3];
        //     done_msg[0] = '\0';
        //     sprintf(done_msg, "[%d]+ Done\t%s\n", child_num, child_cmd);
        //     display_message(done_msg);
        // }

        // Clean exit  
        if (ret != -1 && (token_count == 0 || (strncmp("exit\0", token_arr[0], 5) == 0))) {
            free_variables();  // frees all memory allocated by malloc for variables
            if (argc == 1) free_processes(process_lst);
            // fprintf(stderr, "SERVER STATUS : %d\n", get_server_status());
            if (get_server_status() != 0) {
                signal(SIGCHLD, empty_handler);
                kill(get_server_pid(), SIGINT);
                // int server_pid = get_server_pid();
                // fprintf(stderr, "KILLED SERVER WITH PROCESS ID : %d\n", server_pid);
            }
            break;
        }

        
        
        if (bg_process) { // is a background process
            
              // int status;
            pid_t pid = fork();

            if (pid < 0){
                display_error("ERROR: Unsuccessful fork.", "");
            }
            else if(pid == 0){ // child process
                // display_message("   Forking right now -- Child Process \n");
                token_arr[token_count - 1] = NULL;
                // for (int i = 0; i < token_count; i++){
                //     printf("%s ", token_arr[i]);
                // } printf("\n");

                // ls -a 
                // token count = 2

                char *command[token_count + 1];
                command[0] = "./mysh";
                for (int i = 1; i <= token_count; i++) {
                    command[i] = token_arr[i - 1];
                }

                // for (int i = 0; i < token_count; i++){
                //     printf("%s ", command[i]);
                // } printf("\n");


                argc = token_count - 1;
                execvp("./mysh", command);
                display_error("ERROR: Background process did not execute.", "");
            }
            else { // parent process
                // display_message("   Forking right now -- Parent Process \n");
                ps_lst new_process = malloc(sizeof(struct ps_lst));
                new_process->pid = pid;
                // TODO: validate this strncpy command
                strncpy(new_process->cmd, token_arr[0], sizeof(token_arr[0]) + 1); 
                new_process->next_process = NULL;

                char cmd_line[MAX_STR_LEN *2]; 
                cmd_line[0] = '\0';               
                for (int i = 0; i < token_count - 1; i++){
                    strcat(cmd_line, token_arr[i]);
                    if (i != token_count - 2){
                        strcat(cmd_line, " ");
                    }
                }
                strncpy(new_process->cmd_lst, cmd_line, MAX_STR_LEN * 2 - 1);

                new_process->num = get_process_num();
                new_process->status = 0;
                add_process(new_process, process_lst);

                char process[MAX_STR_LEN];
                process[0] = '\0';
                sprintf(process, "[%d] %d\n", new_process->num, new_process->pid);
                display_message(process);                
                
                increment_process_num();                    
                argc = 1;
            }
            bg_process = false;
        } else { // no background process
            // printf("NO BACKGROUND PROCESSES\n");
            //If pipe symbol '|' exists, then run the pipe part of the execution
            if(strchr(input_cp, '|')){

                char *cmds[MAX_STR_LEN] = {NULL};
                int pipe_num = unpipify_cmds(input_cp, cmds) - 1;
                pipe_helper(cmds, pipe_num);
            }
            // else, just run command execution
            else {
                // printf("NORMAL COMMAND EXECUTION\n");
                command_execution(&token_count, token_arr);
            }
          
        }

        if (argc > 1) {
            // printf("Done background process\n");
            free_variables();
            free_processes(process_lst);
            break;
        }
    
    }

    return 0;
}

void free_processes(struct ps_lst* process_lst) {
    ps_lst curr = process_lst;

    while (curr != NULL){
        // printf("Currently freeing process: %d\n", curr->pid);
        ps_lst temp = curr;
        curr = curr->next_process;
        free(temp);
    }
}

void add_process(struct ps_lst* new_process, struct ps_lst* process_lst){
    ps_lst curr = process_lst;
    while (curr != NULL){
        if (curr->next_process == NULL){
            curr->next_process = new_process;
            curr = curr->next_process->next_process;
        }
        else {
            curr = curr->next_process;
        }
    }
}
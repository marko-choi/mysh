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

int main(int argc, char* argv[]) {

    char *prompt = "mysh$ ";

    char input_buf[MAX_STR_LEN + 1];
    char input_cp[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    input_cp[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};
    int token_count = 0;
    int ret = 0;

    signal(SIGCHLD, bg_process_done);
    signal(SIGINT, mysh_handler);
    
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
        
        while (token_arr[index] != NULL){
            
            // Checks for & symbol for the LAST argument
            if (strcmp(token_arr[index], "&") == 0){
                if (token_arr[index + 1] != NULL) {
                    display_error("ERROR: Must be & at the end of a command", "");
                } 
                else {
                    bg_process = true;
                }
            }
            index++;
        } 

        // Clean exit  
        if (ret != -1 && (token_count == 0 || (strncmp("exit\0", token_arr[0], 5) == 0))) {
            free_variables();  // frees all memory allocated by malloc for variables
            if (argc == 1) free_processes(process_lst);
            if (get_server_status() != 0) {
                signal(SIGCHLD, empty_handler);
                kill(get_server_pid(), SIGINT);
            }
            break;
        }

        
        
        if (bg_process) { // is a background process
            
            pid_t pid = fork();

            if (pid < 0){
                display_error("ERROR: Unsuccessful fork.", "");
            }
            else if(pid == 0){ // child process

                token_arr[token_count - 1] = NULL;

                char *command[token_count + 1];
                command[0] = "./mysh";
                for (int i = 1; i <= token_count; i++) {
                    command[i] = token_arr[i - 1];
                }

                argc = token_count - 1;
                execvp("./mysh", command);
                display_error("ERROR: Background process did not execute.", "");
            }
            else { // parent process
                
                ps_lst new_process = malloc(sizeof(struct ps_lst));
                new_process->pid = pid;

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
            
            //If pipe symbol '|' exists, then run the pipe part of the execution
            if(strchr(input_cp, '|')){

                char *cmds[MAX_STR_LEN] = {NULL};
                int pipe_num = unpipify_cmds(input_cp, cmds) - 1;
                pipe_helper(cmds, pipe_num);
            }
            // else, just run command execution
            else {
                command_execution(&token_count, token_arr);
            }
          
        }

        if (argc > 1) {
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
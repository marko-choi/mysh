#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "io_helpers.h"


// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
}


/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(char *pre_str, char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}


// ===== Input tokenizing =====

/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1); // Not a sanitizer issue since in_ptr is allocated as MAX_STR_LEN+1
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }
    if (read_len > MAX_STR_LEN) {
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
        int junk = 0;
        while((junk = getchar()) != EOF && junk != '\n');
    }
    in_ptr[read_len] = '\0';
    return retval;
}

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens) {
    // in_ptr = "echo hello" - array to the entire word
    // *in_ptr = "e" - refers to the first letter of the word

    char *curr_ptr = strtok (in_ptr, DELIMITERS);
    
    // curr_ptr = "echo" - refers to the first word
    // *curr_ptr = "e" - refers to the first letter of the first word

    // strtok - returns a pointer to the next token / NULL if there are no more tokens
    size_t token_count = 0;

    while (curr_ptr != NULL) {  
        // Retrieves all words in in_ptr[] and places them in tokens[]
        tokens[token_count] = curr_ptr;
        curr_ptr = strtok(NULL, DELIMITERS);
        token_count = token_count + 1;
    }
    tokens[token_count] = NULL;
    return token_count;
}

size_t unpipify_cmds(char *in_ptr, char **cmds) {
    // in_ptr = "echo hello" - array to the entire word
    // *in_ptr = "e" - refers to the first letter of the word

    char *curr_ptr = strtok(in_ptr, "|");
    
    // curr_ptr = "echo" - refers to the first word
    // *curr_ptr = "e" - refers to the first letter of the first word

    // strtok - returns a pointer to the next token / NULL if there are no more tokens
    size_t cmd_count = 0;

    while (curr_ptr != NULL) {  
        // Retrieves all words in in_ptr[] and places them in tokens[]
        cmds[cmd_count] = curr_ptr;
        curr_ptr = strtok(NULL, "|");
        cmd_count = cmd_count + 1;
    }
    cmds[cmd_count] = NULL;
    return cmd_count;
}
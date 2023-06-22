#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "io_helpers.h"
#include "variables.h"

static int VARIABLE_COUNT = 0;

typedef struct Variables{
    char name[MAX_STR_LEN];
    char value[MAX_STR_LEN];
} var;

var *var_lst[MAX_VAR_NUM];

/*
    Variable Assignment
        Creates new heaps to allocate memory to variables upon called
        malloc(sizeof(char) * total_amount_of_characters )
*/
void assign_value(char* var_name, char* var_val){

    int assignment = 0;
    
    for (int i=0; i < VARIABLE_COUNT; i++){
        // Checks for any pre-assigned var_names that are the same
        if (strncmp(var_lst[i]->name, var_name, strlen(var_name)) == 0){

            // Assigns heap memory for the new value and stores pointer back to var_lst
            strncpy(var_lst[i]->value, var_val, strlen(var_val));
            assignment = 1;
        }
    }

    if (!assignment) {
            var *new_var = (var*) malloc(sizeof(var));
            if (new_var){
                strncpy(new_var->name, var_name, MAX_STR_LEN);
                strncpy(new_var->value, var_val, MAX_STR_LEN); 
            }
            // Adds new variable into var_lst
            if (VARIABLE_COUNT < 1000){
                var_lst[VARIABLE_COUNT] = new_var;
                VARIABLE_COUNT += 1;
            } else {
                // fprintf(stderr, "");
                printf("ERROR: Max variable count reached");
            }
    }


}

char* value_lookup(char* var_name){
    // Return value of variable when called
            // Loops through the NAMES[] list and finds a matching var_name 
            // If var_name is found, get its index, finds corresponding var_val from VALS[]
            // otherwise, returns NULL
    for (int i = 0; i < VARIABLE_COUNT; i++){
        if (strncmp(var_lst[i]->name, var_name, strlen(var_lst[i]->name) + 1) == 0){
            return var_lst[i]->value;
        }
    }
    return NULL;
}

// Freeing all memory
void free_variables(){
    for (int i = 0; i < VARIABLE_COUNT; i++){
        free(var_lst[i]);
    }
}
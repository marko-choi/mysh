#ifndef __VARIABLE_H__
#define __VARIABLE_H__

// .h files contain functions and macros

#define MAX_VAR_NUM 1000


/* Assigns values of var_name = var_val 
 * Prereq: var_name, var_val are NULL terminated string
 */
void assign_value(char* var_name, char* var_val);

/* Prereq: var_name is a NULL terminated string
 * Return: (char*) value stored in var_name
 */
char* value_lookup(char* var_name);

/* Frees all memory of variables created with malloc()
 */
void free_variables();

#endif
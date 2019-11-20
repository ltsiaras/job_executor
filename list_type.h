#ifndef __LIST__TYPE__
#define __LIST__TYPE__
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

typedef struct process
{
	pid_t pid;
	int jobID;
	char **command;
}process;

int equal(process Elem1, char *str);	
int lesser(process Elem1, process Elem2);
int greater(process Elem1, process Elem2);

void set_value (process *target, process source);
void set_value_string(process *target, char **string);
void write_value(int fd, process proc);
#endif

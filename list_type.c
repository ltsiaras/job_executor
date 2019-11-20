#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include "list_type.h"
#include <fcntl.h>
#include <unistd.h>


void set_value (process *target, process source)
{
	int i = 0, j;
	target->pid = source.pid;
	target->jobID = source.jobID;
	/*if (target->command != NULL)
	{
		while (target->command[i] != NULL)
		{
			memset(target->command[i], 0, strlen(target->command[i]) + 1);
		//	free(target->command[i]);
			i++;
		}
		free(target->command);
	}*/

	if (source.command != NULL)
	{
		while (source.command[i] != NULL)
			i++;
		target->command = malloc((i+1)*sizeof(char*));

		for (j = 0; j < i; j++)
		{
			target->command[j] = malloc(strlen(source.command[j]) + 1);
			strncpy(target->command[j], source.command[j], strlen(source.command[j]) + 1);
		}
		target->command[j] = '\0';
	}
}

void set_value_string(process *target, char **string)
{
	int i = 0, j;
	while (string[i] != NULL)
		i++;
	target->command = malloc((i+1)*sizeof(char*));

	for (j = 0; j < i; j++)
	{
		target->command[j] = malloc(strlen(string[j]) + 1);
		strncpy(target->command[j], string[j], strlen(string[j]) + 1);
	}
	target->command[j] = '\0';
}

void write_value(int fd, process proc)
{
	int i = 0, j, length;
	size_t nwrite;

	if ((nwrite = write(fd, &proc.jobID, sizeof(int))) == -1)
	{
		perror("Error writing to fifo");
		exit(5);
	}
	
	while (proc.command[i] != NULL)
		i++;

	if ((nwrite = write(fd, &i, sizeof(int))) == -1)
	{
		perror("Error writing to fifo");
		exit(5);
	}

	for (j = 0; j < i; j++)
	{
		length = strlen(proc.command[j]);

		if ((nwrite = write(fd, &length, sizeof(int))) == -1)
		{
			perror("Error writing to fifo");
			exit(5);
		}		
		
		if ((nwrite = write(fd, proc.command[j], length)) == -1)
		{
			perror("Error writing to fifo");
			exit(5);
		}
	}
}


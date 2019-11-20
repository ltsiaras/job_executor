#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "list.h"


char *fifo = "Server_Fifo";
int jobID = 0, concurrency = 1;

info_p running_list = NULL, queued_list = NULL;
list_node_p current = NULL;

void child_signal_handler(int signo)
{
	int status, chpid, found, error;
	process proc;
	while ((chpid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		proc.pid = chpid;
		LIST_first(running_list, &current, &error);
		LIST_search_pid(running_list, chpid, &proc, &current, &found);
		LIST_delete(&running_list, &current, &error);
	}
	
	while (running_list->size < concurrency && !LIST_empty(queued_list)) 
	{
		LIST_first(queued_list, &current, &error);
		chpid = fork();
		if (chpid == 0)	//command is executed
		{
			execvp(current->data.command[0], current->data.command);
			perror("exec");
		}
		else	//server is running
		{
			proc.jobID = current->data.jobID;
			proc.pid = chpid;
			set_value_string(&proc, current->data.command);
			current = NULL;
			LIST_insert(&running_list, proc, current, &error);
			LIST_last(running_list, &current, &error);
			printf("Process with pid %d and jobID %d is executing\n", (int)current->data.pid, current->data.jobID);

			LIST_first(queued_list, &current, &error);
			LIST_delete(&queued_list, &current, &error);
		}
	}
}



void user_signal_handler(int signo)	//handler for Commander to Server signal
{
	static struct sigaction act2;

	act2.sa_handler = child_signal_handler;

	sigfillset(&(act2.sa_mask));

	sigaction(SIGCHLD, &act2, NULL);

	int fd_server, fd_client, i, length, comm_num, arg, running, error;
	size_t nread, nwrite;
	pid_t pid;
	char *buffer, **command;
	process proc;

	if (running_list == NULL)
		running_list = LIST_create();
	if (queued_list == NULL)
		queued_list = LIST_create();

	if ((fd_server = open(fifo, O_RDONLY)) == -1)
	{
		perror("Error opening fifo for reading");
		exit(2);
	}

	if ((nread = read(fd_server, &comm_num, sizeof(int))) == -1)
	{
		perror("Problem reading from fifo");
		exit(4);
	}

	if (comm_num == 1)	//command 1: issuejob
	{
		if ((nread = read(fd_server, &arg, sizeof(int))) == -1)
		{
			perror("Problem reading from fifo");
			exit(4);
		}
		command = malloc((arg+1)*sizeof(char*));
		for(i = 0; i < arg; i++)
		{
			if ((nread = read(fd_server, &length, sizeof(int))) == -1)
			{
				perror("Problem reading from fifo");
				exit(4);
			}
			buffer = malloc(length + 1);
			memset(buffer, 0, strlen(buffer));
			command[i] = malloc(length + 1);
			if ((nread = read(fd_server, buffer, length)) == -1)
			{
				perror("Problem reading from fifo");
				exit(4);
			}
			strcat(buffer,"\0");
			strcpy(command[i], buffer);
			memset(buffer, 0, strlen(buffer));
			free(buffer);
		}
		command[arg] = NULL;
		close(fd_server);

		jobID++;
		/*** Jobs can be executed and no other is waiting in queue ***/
		if (running_list->size < concurrency && LIST_empty(queued_list))
		{
			pid = fork();
			if (pid == 0)	//command is executed
			{
				execvp(command[0], command);
				perror("exec");
			}
			else	//server is running
			{
				proc.jobID = jobID;
				proc.pid = pid;
				set_value_string(&proc, command);
				current = NULL;
				LIST_insert(&running_list, proc, current, &error);
				LIST_last(running_list, &current, &error);
				printf("Process with pid %d and jobID %d is executing\n", (int)current->data.pid, current->data.jobID);
				running = 1;
				
				for (i = 0; i < arg; i++)
				{
					memset(command[i], 0, strlen(command[i]) + 1);
					free(command[i]);
				}
				free(command);

				if ((fd_client = open(fifo, O_WRONLY)) == -1)
				{
					perror("Error opening fifo for writing");
					exit(3);
				}

				if ((nwrite = write(fd_client, &jobID, sizeof(int))) == -1)
				{
					perror("Error writing to fifo");
					exit(5);
				}
				if ((nwrite = write(fd_client, &running, sizeof(int))) == -1)
				{
					perror("Error writing to fifo");
					exit(5);
				}
				close(fd_client);
			}
		}
		/*** Jobs can be executed but other jobs are waiting in queue ***/ 
		else if (running_list->size < concurrency && !LIST_empty(queued_list)) 
		{
			proc.jobID = jobID;
			proc.pid = 0;
			current = NULL;
			set_value_string(&proc, command);
			LIST_insert(&queued_list, proc, current, &error);	//putting new job in waiting queue
			LIST_last(queued_list, &current, &error);
			printf("Process with pid %d and jobID %d is queued\n", (int)current->data.pid, current->data.jobID);
			running = 0;
			
			for (i = 0; i < arg; i++)
			{
				memset(command[i], 0, strlen(command[i]));
				free(command[i]);
			}
			free(command);

			if ((fd_client = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo for writing");
				exit(3);
			}

			if ((nwrite = write(fd_client, &jobID, sizeof(int))) == -1)
			{
				perror("Error writing to fifo");
				exit(5);
			}
			if ((nwrite = write(fd_client, &running, sizeof(int))) == -1)
			{
				perror("Error writing to fifo");
				exit(5);
			}
			close(fd_client);

			/*** Executing first job in queue ***/
			LIST_first(queued_list, &current, &error);
			pid = fork();
			if (pid == 0)	//command is executed
			{
				execvp(current->data.command[0], current->data.command);
				perror("exec");
			}
			else	//server is running
			{
				proc.jobID = current->data.jobID;
				proc.pid = pid;
				set_value_string(&proc, current->data.command);
				current = NULL;
				LIST_insert(&running_list, proc, current, &error);
				LIST_last(running_list, &current, &error);
				printf("Process with pid %d and jobID %d is executing\n", (int)current->data.pid, current->data.jobID);

				LIST_first(queued_list, &current, &error);
				LIST_delete(&queued_list, &current, &error);
			}
		}
		/*** No more jobs can be run ***/
		else if (running_list->size >= concurrency)
		{
			proc.jobID = jobID;
			proc.pid = 0;
			current = NULL;
			set_value_string(&proc, command);
			LIST_insert(&queued_list, proc, current, &error);
			LIST_last(queued_list, &current, &error);
			printf("Process with pid %d and jobID %d is queued\n", (int)current->data.pid, current->data.jobID);
			running = 0;

			if ((fd_client = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo for writing");
				exit(3);
			}

			if ((nwrite = write(fd_client, &jobID, sizeof(int))) == -1)
			{
				perror("Error writing to fifo");
				exit(5);
			}
			if ((nwrite = write(fd_client, &running, sizeof(int))) == -1)
			{
				perror("Error writing to fifo");
				exit(5);
			}
			close(fd_client);
		}
	}
	else if (comm_num == 2)	//command 2: setConcurrency
	{
		if ((nread = read(fd_server, &concurrency, sizeof(int))) == -1)
		{
			perror("Problem reading from fifo");
			exit(4);
		}
		
		close(fd_server);

		while ((running_list->size < concurrency) && !(LIST_empty(queued_list))) 
		{
			/*** Executing first job in queue ***/
			LIST_first(queued_list, &current, &error);
			pid = fork();
			if (pid == 0)	//command is executed
			{
				execvp(current->data.command[0], current->data.command);
				perror("exec");
			}
			else	//server is running
			{
				proc.jobID = current->data.jobID;
				proc.pid = pid;
				set_value_string(&proc, current->data.command);
				current = NULL;
				LIST_insert(&running_list, proc, current, &error);
				LIST_last(running_list, &current, &error);
				printf("Process with pid %d and jobID %d is executing\n", (int)current->data.pid, current->data.jobID);

				LIST_first(queued_list, &current, &error);
				LIST_delete(&queued_list, &current, &error);
			}
		}
	}
	else if (comm_num == 3)	//stop
	{
		int jobID_stop, found, pid;
		process proc;
	
		if ((nread = read(fd_server, &jobID_stop, sizeof(int))) == -1)
		{
			perror("Problem reading from fifo");
			exit(4);
		}
	
		close(fd_server);

		current = NULL;
		pid = LIST_search_jobID(running_list, jobID_stop, &proc, &current, &found);
		if (found == 0)
		{
			pid = LIST_search_jobID(queued_list, jobID_stop, &proc, &current, &found);
			if (found != 0)
			{
				LIST_delete(&queued_list, &current, &error);
			}
		}
		else
		{
			LIST_delete(&running_list, &current, &error);
			if ((error = kill(pid, SIGKILL)) == -1)
			{
				perror("Problem sending signal to Server");
				exit(6);
			}
		}
	}
	else if (comm_num == 4)	//poll
	{
		if ((nread = read(fd_server, &running, sizeof(int))) == -1)
		{
			perror("Problem reading from fifo");
			exit(4);
		}
	
		close(fd_server);

		if ((fd_client = open(fifo, O_WRONLY)) == -1)
		{
			perror("Error opening fifo for writing");
			exit(3);
		}

		if (running == 1)	//poll running
		{
			if ((nwrite = write(fd_client, &(running_list->size), sizeof(int))) == -1)
			{
				perror("Problem in writing to fifo");
				exit(5);
			}	

			if (running_list->size != 0)
			{
				int i, error;
				process proc;
				current = NULL;
				LIST_first(running_list, &current, &error);
				for (i = 0; i < running_list->size; i++)
				{
					LIST_content(running_list, current, &proc, &error);
					if ((nwrite = write(fd_client, &proc.jobID, sizeof(int))) == -1)
					{
						perror("Error writing to fifo");
						exit(5);
					}
					LIST_next(running_list, &current, &error);
				}
			}
		}
		else	//poll queued
		{
			if ((nwrite = write(fd_client, &(queued_list->size), sizeof(int))) == -1)
			{
				perror("Problem in writing to fifo");
				exit(5);
			}	

			if (queued_list->size != 0)
			{
				int i, error;
				process proc;
				current = NULL;
				LIST_first(queued_list, &current, &error);
				for (i = 0; i < queued_list->size; i++)
				{
					LIST_content(queued_list, current, &proc, &error);
					if ((nwrite = write(fd_client, &proc.jobID, sizeof(int))) == -1)
					{
						perror("Error writing to fifo");
						exit(5);
					}
					LIST_next(queued_list, &current, &error);
				}
			}
		}

		close(fd_client);
	}
	else if (comm_num == 5)	//exit
	{
		int pid, status;

		close(fd_server);

		LIST_destroy(&queued_list);
		while ((pid = waitpid(-1, &status, 0)) > 0)
		{
			;
		}
		LIST_destroy(&running_list);
		unlink("Server_File.txt");
	//	unlink(fifo);
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	size_t nwrite;
	int pid, fd_file;

	static struct sigaction act1, act2;

	act1.sa_handler = user_signal_handler;

	sigfillset(&(act1.sa_mask));

	sigaction(SIGRTMIN, &act1, NULL);

	act2.sa_handler = child_signal_handler;

	sigfillset(&(act2.sa_mask));

	sigaction(SIGCHLD, &act2, NULL);

	pid = getpid();
	char buf[10];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", pid);
	if ((fd_file = open("Server_File.txt", O_CREAT | O_WRONLY, 0666)) == -1 && errno != EEXIST)
	{
		perror("Error creating file");
		exit(1);
	}

	if ((nwrite = write(fd_file, buf, sizeof(buf))) == -1)
	{
		perror("Error writing in file");
		exit(8);
	}

	close(fd_file);

	while(1)
	{	
		pause();
	}

}

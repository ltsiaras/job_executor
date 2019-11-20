#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define TRUE 1

char *fifo = "Server_Fifo";

int main(int argc, char *argv[])
{
	int fd_server, fd_client, length, jobID, i, fd_file, new_length = 0, command, running, error, pid, conc, arg;
	size_t nread, nwrite;
	char *job = NULL, *usage[5] = {"issuejob <job>", "setConcurrency <N>", "stop <jobID>", "poll [running,queued]", "exit"};

	if (argc <= 1)
	{
		fprintf(stderr, "Usage:  %s\n", usage[0]);
		for (i = 1; i < 5; i++)
			fprintf(stderr, "\t%s\n", usage[i]);
		exit(1);
	}
	
	if (mkfifo(fifo , 0666) == -1 && errno != EEXIST)
	{
		perror("jobCommander:mkfifo");
		exit(2);
	}

	/*** JobExecutorServer is created ***/
	if ((fd_file = open("Server_File.txt", O_RDONLY, 0666)) != -1 && errno == EEXIST)
	{
		/*** Reading Server PID ***/
		char buf[10];
		if ((nread = read(fd_file, &buf, sizeof(buf))) == -1)
		{
			perror("Problem reading from fifo");
			exit(5);
		}
		pid = atoi(buf);
		close(fd_file);

		/****Sends signal to server****/
		if ((error = kill(pid, SIGRTMIN)) == -1)
		{
			perror("Problem sending signal to Server");
			exit(6);
		}

		if (strcmp(argv[1], "issuejob") == 0)	//command to issue a job
		{
			if ((fd_server = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo");
				exit(3);
			}
	
			command = 1;
			if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}
		
			arg = argc - 2;
			if ((nwrite = write(fd_server, &arg, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}

			for (i = 2; i < argc; i++)
			{
				length = strlen(argv[i]);
				new_length += length;

				if ((nwrite = write(fd_server, &length, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}

				if ((nwrite = write(fd_server, argv[i], length)) == -1)
				{
					perror("Error writing to fifo");
					exit(4);
				}
				job = realloc(job, new_length + 1);
				strcat(job, argv[i]);
				if (i != argc - 1)
					strcat(job, " ");
			}
			close(fd_server);
			strcat(job, "\0");

			if ((fd_client = open(fifo, O_RDONLY)) == -1)
			{
				perror("Error opening fifo for reading");
				exit(2);
			}

			if ((nread = read(fd_client, &jobID, sizeof(int))) == -1)
			{
				perror("Problem reading from fifo");
				exit(5);
			}
	
			if ((nread = read(fd_client, &running, sizeof(int))) == -1)
			{
				perror("Problem reading from fifo");
				exit(5);
			}

			if (running == TRUE)
				printf("<%d,%s> is running\n", jobID, job);
			else
				printf("<%d,%s> is queued\n", jobID, job);
			close(fd_client);
			exit(0);
		}
		else if (strcmp(argv[1], "setConcurrency") == 0)	//command to set concurrency
		{
			if ((fd_server = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo");
				exit(3);
			}
	
			command = 2;
			if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}
			
			conc = atoi(argv[2]);
			if ((nwrite = write(fd_server, &conc, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}
			close(fd_server);
			exit(0);
		}
		else if (strcmp(argv[1], "stop") == 0)	//stop command
		{
			int jobID_stop;

			if ((fd_server = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo");
				exit(3);
			}
	
			command = 3;
			if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}

			jobID_stop = atoi(argv[2]);
			if ((nwrite = write(fd_server, &jobID_stop, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}
			close(fd_server);
			exit(0);
		}
		else if (strcmp(argv[1], "poll") == 0)	//poll command
		{
			int size, j;

			if ((fd_server = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo");
				exit(3);
			}
	
			command = 4;

			if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}

			if (strcmp(argv[2], "running") == 0)	//poll running
			{
				running = 1;
				if ((nwrite = write(fd_server, &running, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}
				close(fd_server);
			}
			else if (strcmp(argv[2], "queued") == 0)	//poll queued
			{
				running = 0;
				if ((nwrite = write(fd_server, &running, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}
				close(fd_server);
			}
			else
			{
				fprintf(stderr, "Usage:  %s\n", usage[0]);
				for (i = 1; i < 5; i++)
					fprintf(stderr, "\t%s\n", usage[i]);
				exit(1);
			}

			if ((fd_client = open(fifo, O_RDONLY)) == -1)
			{
				perror("Error opening fifo for reading");
				exit(2);
			}

			if ((nread = read(fd_client, &size, sizeof(int))) == -1)	//getting list size
			{
				perror("Problem reading from fifo");
				exit(5);
			}

			if (size == 0 && running == 1)
			{
				printf("No jobs are running\n");
				exit(0);
			}
			else if (size == 0 && running == 0)
			{
				printf("No jobs are queued\n");
				exit(0);			
			}
			
			if (running == 1)
					printf("Jobs that are running are:\n");
			else
				printf("Jobs that are queued are:\n");

			for (j = 0; j < size; j++)
			{
				if ((nread = read(fd_client, &jobID, sizeof(int))) == -1)
				{
					perror("Problem reading from fifo");
					exit(5);
				}

				printf("%d\n", jobID);
			}
			close(fd_client);
		}
		else if (strcmp(argv[1], "exit") == 0)	//exit command
		{
			if ((fd_server = open(fifo, O_WRONLY)) == -1)
			{
				perror("Error opening fifo");
				exit(3);
			}
	
			command = 5;

			if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
			{
				perror("Problem writing to fifo");
				exit(4);
			}

			close(fd_server);
			exit(0);
		}
		else
		{
			fprintf(stderr, "Usage:  %s\n", usage[0]);
			for (i = 1; i < 5; i++)
				fprintf(stderr, "\t%s\n", usage[i]);
			exit(1);
		}
	}
	else if(errno != EEXIST)	//file does not exist
	{
		pid = fork();
		if (pid == 0)	//server is created
		{
			execlp("./jobExecutorServer", "jobExecutorServer", NULL);
			perror("Couldn't use execlp");
			exit(7);
		}
		else	//parent
		{
			printf("%d\n", pid);
			sleep(1);
			if ((error = kill(pid, SIGRTMIN)) == -1)
			{
				perror("Problem sending signal to Server");
				exit(6);
			}

			if (strcmp(argv[1], "issuejob") == 0)
			{
				if ((fd_server = open(fifo, O_WRONLY)) == -1)
				{
					perror("Error opening fifo");
					exit(3);
				}
		
				command = 1;
				if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}
				arg = argc - 2;
				if ((nwrite = write(fd_server, &arg, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}

				for (i = 2; i < argc; i++)
				{
					length = strlen(argv[i]);
					new_length += length;

					if ((nwrite = write(fd_server, &length, sizeof(int))) == -1)
					{
						perror("Problem writing to fifo");
						exit(4);
					}

					if ((nwrite = write(fd_server, argv[i], length)) == -1)
					{
						perror("Error writing to fifo");
						exit(4);
					}
					job = realloc(job, new_length + 1);
					strcat(job, argv[i]);
					if (i != argc - 1)
						strcat(job, " ");
				}
				close(fd_server);
				strcat(job, "\0");

				if ((fd_client = open(fifo, O_RDONLY)) == -1)
				{
					perror("Error opening fifo for reading");
					exit(2);
				}

				if ((nread = read(fd_client, &jobID, sizeof(int))) == -1)
				{
					perror("Problem reading from fifo");
					exit(5);
				}
		
				if ((nread = read(fd_client, &running, sizeof(int))) == -1)
				{
					perror("Problem reading from fifo");
					exit(5);
				}

				if (running == TRUE)
					printf("<%d,%s> is running\n", jobID, job);
				else
					printf("<%d,%s> is queued\n", jobID, job);
				close(fd_client);
				exit(0);
			}
			else if (strcmp(argv[1], "setConcurrency") == 0)	//command to set concurrency
			{
				if ((fd_server = open(fifo, O_WRONLY)) == -1)
				{
					perror("Error opening fifo");
					exit(3);
				}
	
				command = 2;
				if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}

				conc = atoi(argv[2]);
				if ((nwrite = write(fd_server, &conc, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}
				close(fd_server);
				exit(0);
			}
			else if (strcmp(argv[1], "stop") == 0)
			{
				if ((fd_server = open(fifo, O_WRONLY)) == -1)
				{
					perror("Error opening fifo");
					exit(3);
				}
	
				command = 3;
				if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}

				if ((nwrite = write(fd_server, argv[2], sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}
				close(fd_server);
				exit(0);
			}
			else if (strcmp(argv[1], "poll") == 0)
			{
				int size, j;

				if ((fd_server = open(fifo, O_WRONLY)) == -1)
				{
					perror("Error opening fifo");
					exit(3);
				}
	
				command = 4;

				if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}

				if (strcmp(argv[2], "running") == 0)
				{
					running = 1;
					if ((nwrite = write(fd_server, &running, sizeof(int))) == -1)
					{
						perror("Problem writing to fifo");
						exit(4);
					}
					close(fd_server);
				}
				else if (strcmp(argv[2], "queued") == 0)
				{
					running = 0;
					if ((nwrite = write(fd_server, &running, sizeof(int))) == -1)
					{
						perror("Problem writing to fifo");
						exit(4);
					}
					close(fd_server);
				}
				else
				{
					fprintf(stderr, "Usage:  %s\n", usage[0]);
					for (i = 1; i < 5; i++)
						fprintf(stderr, "\t%s\n", usage[i]);
					exit(1);
				}

				if ((fd_client = open(fifo, O_RDONLY)) == -1)
				{
					perror("Error opening fifo for reading");
					exit(2);
				}

				if ((nread = read(fd_client, &size, sizeof(int))) == -1)
				{
					perror("Problem reading from fifo");
					exit(5);
				}

				if (size == 0 && running == 1)
				{
					printf("No jobs are running\n");
					exit(0);
				}
				else if (size == 0 && running == 0)
				{
					printf("No jobs are queued\n");
					exit(0);			
				}

				for (j = 0; j < size; j++)
				{
					if ((nread = read(fd_client, &jobID, sizeof(int))) == -1)
					{
						perror("Problem reading from fifo");
						exit(5);
					}

					printf("%d\n", jobID);
				}
				close(fd_client);
			}
			else if (strcmp(argv[1], "exit") == 0)
			{
				if ((fd_server = open(fifo, O_WRONLY)) == -1)
				{
					perror("Error opening fifo");
					exit(3);
				}
	
				command = 5;

				if ((nwrite = write(fd_server, &command, sizeof(int))) == -1)
				{
					perror("Problem writing to fifo");
					exit(4);
				}

				close(fd_server);
				exit(0);
			}
			else
			{
				fprintf(stderr, "Usage:  %s\n", usage[0]);
				for (i = 1; i < 5; i++)
					fprintf(stderr, "\t%s\n", usage[i]);
				exit(1);
			}
		}
	}
	return 0;
}

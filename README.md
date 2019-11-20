# README

The job_executor application consists of two executables, the jobCommander and the jobExecutorServer. The jobCommander is run first and 
initializes settings for the jobExecutorServer. Then through the jobCommander you can issue jobs (processes to execute) to 
the jobExecutorServer, poll the current job queue, set the concurrency of execution and stop the jobExecutorServer.

The communication is done with signals and pipes.

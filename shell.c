#include "shell.h"

void ExecCmd(struct Command *command)
{
	int i, j, status;
	const int numProcs = command->num_sub_commands;
	const int numPipes = numProcs-1;
	Pipe *pipes = malloc(sizeof(Pipe)*numPipes);
	pid_t *pids = malloc(sizeof(pid_t)*numProcs), pid;

	for (i = 0; i < numPipes; ++i)
	{
		status = pipe(pipes[i].fd);
		assert(status == 0);
	}

	for (i = 0; i < numProcs; ++i)
	{
		pid = fork();
		pids[i] = pid;
		assert(pid >= 0);

		if (pid == 0) // Child process
		{
			// Stdin redirect (<)
			if (command->stdin_redirect)	
			{
				int infile = open(command->stdin_redirect, \
							 O_RDONLY);
				if (infile < 0)
				{
					printf("%s: File does not exist!\n", command->stdin_redirect);
					exit(-1);
				}
				close(0);
				dup(infile);
			}
			// Stdout redirect (>)
			if (command->stdout_redirect)	
			{
				int outfile = open(command->stdout_redirect, \
							  O_WRONLY | O_APPEND | O_CREAT, S_IRWXU); // chmod 700
				if (outfile < 0)
				{
					printf("%s: Cannot write to file!\n", command->stdout_redirect);
					exit(-1);
				}
				close(1);
				dup(outfile);
			}
			// Close stdout && stderr if background and no redirect
			// Technically not standard shell behavior
			if (command->background && !command->stdout_redirect) 
			{
				close(1);
				close(2);
			}
			// Close every other pipes
			for (j = 0; j < numPipes; ++j)
			{
				if (j != i-1)
				{
					close(pipes[j].fd[0]);
				}
				if (j != i)
				{
					close(pipes[j].fd[1]);
				}
			}
			// Piping
			if (i != numPipes) // If not the last process then close output
			{
				close(1);
				status = dup(pipes[i].fd[1]);
				assert(status >= 0);
			}
			if (i != 0)	// If not the first process then close input
			{
				close(0);
				status = dup(pipes[i-1].fd[0]);
				assert(status >= 0);
			}
			// EXECUTE
			status = execvp(command->sub_commands[i].argv[0], command->sub_commands[i].argv);
			if (status < 0)	// If execvp failed
			{
				printf("%s: Command not found!\n", command->sub_commands[i].argv[0]);
				exit(-1);
			}
		} 
	}
	for (j = 0; j < numPipes; ++j)
	{
		close(pipes[j].fd[0]);
		close(pipes[j].fd[1]);
	}
	if (command->background)
	{
		printf("[%d]\n", pids[numProcs-1]);
		BackgroundWait(pids[numProcs-1]);
	} 
	else
	{
		for (i = 0; i < numProcs; ++i)
			waitpid(pids[i], &status, 0);
	}
	free(pipes);
	free(pids);
}

int main()
{
	char s[KILOBYTE], cwd[KILOBYTE];
	struct Command *command = malloc(sizeof(struct Command));
	while (1)
	{
		memset(command, 0, sizeof(struct Command));
		getcwd(cwd, sizeof(cwd));
		printf("%s $ ", cwd);
		fgets(s, sizeof(s), stdin);
		// Extract arguments and print them
		ReadCommand(s, command);
		// PrintCommand(command);
		BackgroundCheck();
		// Run shit
		InterpretCmd(command);
	}
}
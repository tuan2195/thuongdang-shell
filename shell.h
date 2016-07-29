#include <stdio.h>		// printf(), getchar()
#include <stdlib.h>		// free(), environ
#include <string.h>		// memset(), malloc()
#include <unistd.h> 	// execvp()
#include <sys/types.h>	// pid_t
#include <sys/wait.h>	// wait()
#include <fcntl.h>		// open()
#include <assert.h>		// assert()

#define MAX_SUB_COMMANDS 5
#define MAX_ARGS 10
#define KILOBYTE 1<<10
#define RESET -1

extern char **environ;

typedef struct _pipe_
{
	int fd[2];
} Pipe;

struct SubCommand
{
	char *line;
	char *argv[MAX_ARGS];
};

struct Command
{
	struct SubCommand sub_commands[MAX_SUB_COMMANDS];
	int num_sub_commands;
	char *stdin_redirect;
	char *stdout_redirect;
	int background;
};

void ChangeDir(char *path);
void PrintArgs(char **argv);
void ReadArgs(char *in, char **argv, int size);
void ReadCommand(char *line, struct Command *command);
void PrintCommand(struct Command *command);
void ReadRedirectsAndBackground(struct Command *command);
void ArrayShift(char** argv, int pos);
void ExecCmd(struct Command *command);
void InterpretCmd(struct Command *command);
int  BackgroundWait(int external_pid);
void BackgroundCheck(void);
void ListEnvVar(void);
void Export(struct Command *command);
void Snake(void);
void About(void);
void Initialize(void);

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

void Initialize(void)
{
	BackgroundWait(RESET);
}

void ChangeDir(char *path)
{
	int status = chdir(path);

	if (status < 0)	// Final check
		printf("%s: Path does not exist!\n", path);
}

void ReadRedirectsAndBackground(struct Command *command)
{
	int i = 0, j = 0;
	command->background = 0;

	for (i = 0; i < command->num_sub_commands; ++i)
	{
		j = 0;
		while (command->sub_commands[i].argv[j]) 
		{
			if (strcmp(command->sub_commands[i].argv[j],"<")==0)
			{
				if (command->sub_commands[i].argv[j-1])
				{ 
					command->stdin_redirect = command->sub_commands[i].argv[j+1];
				}
				ArrayShift(command->sub_commands[i].argv, j);
				ArrayShift(command->sub_commands[i].argv, j);
				continue;
			}

			if (strcmp(command->sub_commands[i].argv[j],">")==0)
			{
				if (command->sub_commands[i].argv[j+1])
				{
					command->stdout_redirect = command->sub_commands[i].argv[j+1];
				}
				ArrayShift(command->sub_commands[i].argv, j);
				ArrayShift(command->sub_commands[i].argv, j);
				continue;
			}

			if (strcmp(command->sub_commands[i].argv[j],"&")==0)
			{
				command->background = 1;
				command->sub_commands[i].argv[j] = NULL;
				continue;
			}
			++j;
		}
	}

} // ReadRedirectsAndBackground

void ArrayShift(char** argv, int pos)
{
	while (argv[pos]) 
	{
		argv[pos]=argv[pos+1];
		pos++;
	}
	
	argv[pos] = NULL;
}


void PrintArgs(char **argv)
{
	int i = 0;
	while(argv[i]!=NULL)
	{
		printf("argv[%d] = '%s'\n", i, argv[i]);
		++i;
	}
}

void ReadArgs(char *in, char **argv, int size)
{
	int index = 0;
	char *temp = strdup(in);
	char *token, *delim = " \n";

	token = strtok(temp, delim);
	while (token != NULL && index < size-1)
	{
		argv[index++] = strdup(token);
		token = strtok(NULL, delim);
	}

	argv[index] = NULL;
	free(temp);
}

void ReadCommand(char *line, struct Command *command)
{
	int index = 0;
	char *token, *delim = "|\n";
	token = strtok(line, delim);
	
	memset(command, 0, sizeof(struct Command));

	while (token != NULL && index < MAX_SUB_COMMANDS)
	{
		command->sub_commands[index].line = strdup(token);
		token = strtok(NULL, delim);
		index++;
	}
	command->num_sub_commands = index;

	for (index = 0; index < command->num_sub_commands; ++index)
	{
		ReadArgs(command->sub_commands[index].line, command->sub_commands[index].argv, MAX_ARGS);
	}

	ReadRedirectsAndBackground(command);
};

void PrintCommand(struct Command *command)
{
	int i;

	for (i = 0; i < command->num_sub_commands; ++i)
	{
		printf("Command #%d\n", i+1);
		printf("Command line:\n%s\n", command->sub_commands[0].line);
		PrintArgs(command->sub_commands[i].argv);
	}

	printf("Redirect stdin: ");
	if (command->stdin_redirect)
		printf("%s\n", command->stdin_redirect);
	else
		printf("N/A\n");

	printf("Redirect stdout: ");
	if (command->stdout_redirect)
		printf("%s\n", command->stdout_redirect);
	else
		printf("N/A\n");

	printf("Runs in background: ");
	if (command->background)
		printf("Yes\n");
	else
		printf("No\n");
}

void InterpretCmd(struct Command *command)
{
	if (command->sub_commands[0].argv[0] == NULL) // empty line
		return; // avoid segfault

	if (strcmp(command->sub_commands[0].argv[0], "exit") == 0) // exit to exit
		exit(0);
	
	if (strcmp(command->sub_commands[0].argv[0], "cd") == 0) // cd emulator
	{
		ChangeDir(command->sub_commands[0].line+3);
		return;
	}

	if (strcmp(command->sub_commands[0].argv[0], "clear") == 0) // Export environment variables
	{
		printf("\e[1;1H\e[2J");
		return;
	}

	if (strcmp(command->sub_commands[0].argv[0], "export") == 0) // Export environment variables
	{
		Export(command);
		return;
	}

	if (strcmp(command->sub_commands[0].argv[0], "lsenv") == 0) // List environment variables
	{
		ListEnvVar();
		return;
	}

	if (strcmp(command->sub_commands[0].argv[0], "ver") == 0) // version
	{
		About();
		return;
	}

	if (strcmp(command->sub_commands[0].argv[0], "snake") == 0) // easter egg
	{
		Snake();
		return;
	}

	ExecCmd(command);
}

void ListEnvVar(void)
{
	int i = 0;
	while(environ[i])
	{
		printf("%s\n", environ[i]);
		++i;
	}
}

void Export(struct Command *command)
{
	int i = 1, status = 0;

	while(command->sub_commands[0].argv[i])
	{
		status = putenv(command->sub_commands[0].argv[i]);
		assert(status == 0);
		++i;
	}
}

int BackgroundWait(int external_pid)
{
	static int local_pid;
	if (external_pid != 0)
		local_pid = external_pid;
	else if (local_pid != RESET)
		return waitpid(local_pid, NULL, WNOHANG);
	return 0;
}

void BackgroundCheck(void)
{
	int background_pid = BackgroundWait(0);
	if (background_pid) 
	{
		BackgroundWait(RESET); // Reset
		printf("[%d] finished\n", background_pid);
	}
}


void PrintPrompt()
{
	static char cwd[KILOBYTE];
	getcwd(cwd, KILOBYTE);
	printf("%s $ ", cwd);
}

void KBInput(char* buffer)
{
	fgets(buffer, KILOBYTE, stdin);
}

void About(void)
{
	printf("Thuong Dang Shell - #TD$ v. 0.01a\n");
	printf("Coded by Tuan Dao\n");
}
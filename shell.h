#include <stdio.h>		// printf(), getchar()
#include <stdlib.h>		// free()
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
int  BackgroundWait(int pid);
void BackgroundCheck(void);
void Snake(void);

void ChangeDir(char *path)
{
	int status = chdir(path);
	// if (path[0]=='/') // Absolute path
	// {
	// 	status = chdir(path);
	// }
	// else if (path[0]=='.' && path[1]=='/') // ./*
	// {
	// 	status = chdir(path+2);
	// }
	// else // Relative path
	// {
	// 	status = chdir(path);
	// 	// char cwd[KILOBYTE];
	// 	// getcwd(cwd, sizeof(cwd));
	// 	// cwd[strlen(cwd)] = '/';
	// 	// strcat(cwd, path);
	// 	// status = chdir(cwd);
	// 	// printf("CD to %s\n", cwd);
	// }

	if (status < 0)	// Final check
	{
		printf("%s: Path does not exist!\n", path);
	}
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
		argv[pos]=argv[++pos];

	argv[pos] = NULL;
}


void PrintArgs(char **argv)
{
	int i = 0;
	while(argv[i]!=NULL)
	{
		printf("argv[%d] = '%s'\n", i, argv[i]);
		i++;
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
	
	if (strcmp(command->sub_commands[0].argv[0], "snake") == 0) // easter egg
	{
		Snake();
		return;
	}

	if (strcmp(command->sub_commands[0].argv[0], "cd") == 0) // cd emulator
	{
		ChangeDir(command->sub_commands[0].line+3);
		return;
	}

	ExecCmd(command);
}

int BackgroundWait(int external_pid)
{
	static int local_pid;
	if (external_pid != 0)
		local_pid = external_pid;
	else
		if (local_pid != RESET)
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

void Snake(void)
{
	printf("\
	+-------------------------------------------------+\n\
	|Dedicated to someone I had loved, and still love*|\n\
	|     with all my heart. Or what's left of it.    |\n\
	|            (*: As of compile time)              |\n\
	|                ===========> </3                 |\n\
	+-------------------------------------------------+\n" \
	);
	// How do I compile C and C++ code together?...
}
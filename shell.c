#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 	// execvp()
#include <sys/types.h>
#include <sys/wait.h>	// wait()
#include <fcntl.h>		// open()
#include "shell.h"

// struct SubCommand
// {
// 	char *line;
// 	char *argv[MAX_ARGS];
// };

// struct Command
// {
// 	struct SubCommand sub_commands[MAX_SUB_COMMANDS];
// 	int num_sub_commands;
// 	char *stdin_redirect;
// 	char *stdout_redirect;
// 	int background;
// };

void ExecCmd(struct Command *command)
{
	int i, pid, child_status, infile, outfile;

	if (strcmp(command->sub_commands[0].argv[0], "cd") == 0) // cd emulator
	{
		ChangeDir(command->sub_commands[0].line+3);
		return;
	}

	for (i = 0; i < command->num_sub_commands; ++i)
	{
		pid = fork();

		if (pid == 0) // Child process
		{
			if (command->stdin_redirect)	// Stdin redirect (<)
			{
				infile = 	open(command->stdin_redirect, 
							O_RDONLY);
				if (infile < 0)
				{
					printf("%s: File does not exist!\n", command->stdin_redirect);
					exit(-1);
				}
				close(0);
				dup(infile);
			}
			if (command->stdout_redirect)	// Stdin redirect (<)
			{
				outfile = 	open(command->stdout_redirect, \
							O_WRONLY | O_APPEND | O_CREAT, S_IRWXU); // chmod 700
				if (outfile < 0)
				{
					printf("%s: Cannot write to file!\n", command->stdout_redirect);
					exit(-1);
				}
				close(1);
				dup(outfile);
			}
			if (command->background && !command->stdout_redirect) 
			// Close stdout && stderr if & and no redirect
			{
				close(1);
				close(2);
			}
			// EXECUTE
			child_status = execvp(command->sub_commands[i].argv[0], command->sub_commands[i].argv);
			if (child_status < 0)	// If execvp failed
			{
				printf("%s: Command not found!\n", command->sub_commands[i].argv[0]);
				exit(-1);
			}
		} 
		else if (pid > 0)	// Parent
		{
			if (command->background)
				printf("[%d]\n", pid);
			else
				waitpid(pid, &child_status, 0);
		}
	}
}

int main()
{
	char s[KILOBYTE], cwd[KILOBYTE];// path[KILOBYTE];
	struct Command *command;
	command = malloc(sizeof(struct Command));
	while (1)
	{
		memset(command, 0, sizeof(struct Command));
		getcwd(cwd, sizeof(cwd));
		printf("%s $ ", cwd);
		fgets(s, sizeof(s), stdin);
		// Extract arguments and print them
		ReadCommand(s, command);
		PrintCommand(command);
		// Run shit
		ExecCmd(command);
	}
}
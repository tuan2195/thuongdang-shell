#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUB_COMMANDS 5
#define MAX_ARGS 10
#define KILOBYTE 1<<10

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

void ChangeDir(char *path)
{
	int status;
	if (path[0]=='/') // Absolute path
	{
		status = chdir(path);
	}
	else
	{
		char cwd[KILOBYTE];
		getcwd(cwd, sizeof(cwd));
		cwd[strlen(cwd)] = '/';
		strcat(cwd, path);
		status = chdir(cwd);
	}
	if (status < 0)
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
			j++;
		}
	}

} // ReadRedirectsAndBackground

void ArrayShift(char** argv, int pos)
{
	while (argv[pos]) 
		argv[pos]=argv[++pos];

	argv[pos] == NULL;
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
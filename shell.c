#include "shell.h"

int main()
{
	Initialize();
	char buffer[KILOBYTE];
	struct Command *command = malloc(sizeof(struct Command));
	while (1)
	{
		// Print prompt
		PrintPrompt();
		// Get keyboard input
		KBInput(buffer);
		// Check background task
		BackgroundCheck();
		// Extract arguments and print them
		ReadCommand(buffer, command);
		// Run Stuffs
		InterpretCmd(command);
	}
}
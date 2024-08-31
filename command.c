#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "command.h"

void prompt(char *text)
{
	if (text == NULL)
	{
		text = "";
	}

	printf("%s> ", text); 
}

int readCommand(char *buffer, size_t size)
{
	if (fgets(buffer, size, stdin) == NULL)
	{
		if (ferror(stdin))
		{
			perror("readCommand");
		}

		return -1;
	}

	return 0;
}

void parseCommand(struct Command *commands, size_t count, char *cursor)
{
	for (size_t index = 0; index < count; index++)
	{
		struct Command *command = commands + index;

		if (matchToken(&cursor, command->trigger) == 0)
		{
			command->function(cursor);
			return;
		}
	}

	fprintf(stderr, "Undefined command\n\n");
}

#ifndef COMMAND_H
#define COMMAND_H

struct Command
{
	char *trigger;
	void (*function)(char *);
};

void prompt(char *text);
int readCommand(char *, size_t);
void parseCommand(struct Command *, size_t, char *);

#endif

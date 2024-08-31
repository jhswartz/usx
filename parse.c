#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

int parseUInt16(char **cursor, uint16_t *destination)
{
	unsigned long integer = 0;

	if (cursor == NULL)
	{
		return -1;
	}

	if (destination == NULL)
	{
		return -1;
	}

	errno = 0;
	integer = strtoul(*cursor, cursor, 16);

	if (errno)
	{
		return -1;	
	}

	if (!isspace(**cursor) && **cursor != 0)
	{
		return -1;
	}

	*destination = integer;
	return 0;
}

int parseUInt32(char **cursor, uint32_t *destination)
{
	unsigned long integer = 0;

	if (cursor == NULL)
	{
		return -1;
	}

	if (destination == NULL)
	{
		return -1;
	}

	errno = 0;
	integer = strtoul(*cursor, cursor, 16);

	if (errno)
	{
		return -1;	
	}

	if (!isspace(**cursor) && **cursor != 0)
	{
		return -1;
	}

	*destination = integer;
	return 0;
}

int parseFilename(char **cursor, char **filename)
{
	if (cursor == NULL)
	{
		return -1;
	}

	if (filename == NULL)
	{
		return -1;
	}

	while (isspace(**cursor))
	{
		(*cursor)++;
	}

	*filename = *cursor;

	while (!isspace(**cursor))
	{
		if (**cursor == 0)
		{
			break;
		}

		(*cursor)++;
	}

	*(*cursor)++ = 0;
	return 0;
}

int matchToken(char **cursor, char *token)
{
	size_t length = 0;
	
	skipSpace(cursor);
	length = strlen(token);

	if (strncmp(*cursor, token, length) == 0)
	{
		*cursor += length;
		return 0;
	}

	return -1;
}

void skipSpace(char **cursor)
{
	if (cursor && *cursor)
	{
		while (isspace(**cursor))
		{
			(*cursor)++;
		}
	}
}

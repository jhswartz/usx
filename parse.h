#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>

int parseUInt16(char **, uint16_t *);
int parseUInt32(char **, uint32_t *);
int parseFilename(char **, char **);

int matchToken(char **, char *);
void skipSpace(char **);

#endif

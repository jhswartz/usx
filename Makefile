PROGRAM = usx
SOURCES = *.c
CFLAGS  = -pedantic -Wall -g
LDFLAGS = -lusb-1.0

all: main.c
	$(CC) -o $(PROGRAM) $(SOURCES) $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(PROGRAM) 

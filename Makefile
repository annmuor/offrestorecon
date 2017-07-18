CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lselinux
SOURCES=offrestorecon.c queue.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=offrestorecon

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
		$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
		$(CC) $(CFLAGS) $< -o $@

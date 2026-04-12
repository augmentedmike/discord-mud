CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address
LDFLAGS = 
SOURCES = $(wildcard src/*.c)                
OBJECTS = $(SOURCES:.c=.o)

all: world

run: world
	ASAN_OPTIONS=detect_leaks=1 ./world

world: $(OBJECTS)
	$(CC) $(CFLAGS) -o world $(OBJECTS) $(LDFLAGS)

clean:
	rm -f world src/*.o
CC     = gcc
CFLAGS = -Wall -O2 -I./src -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa \
          -framework IOKit -framework CoreAudio -framework CoreVideo

SRC = $(shell find src -name "*.c")
OBJ = $(SRC:.c=.o)

all: beehive

beehive: $(OBJ)
	$(CC) $(OBJ) -o beehive $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find src -name "*.o" -delete
	rm -f beehive

run: all
	./beehive

.PHONY: all clean run

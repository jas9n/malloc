CC = cc
CFLAGS = -Wall -Wextra -Werror -Wno-deprecated-declarations -std=c11

all: demo

demo: main.o allocator.o
	$(CC) $(CFLAGS) -o demo main.o allocator.o

main.o: main.c allocator.h
	$(CC) $(CFLAGS) -c main.c

allocator.o: allocator.c allocator.h
	$(CC) $(CFLAGS) -c allocator.c

clean:
	rm -f *.o demo

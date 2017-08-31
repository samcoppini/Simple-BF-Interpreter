CC=gcc
CFLAGS=$(FLAGS) -Wall -Wextra -Werror -O3 -std=c99 -pedantic

all:
	$(CC) brainfuck.c $(CFLAGS) -o "bf.exe"

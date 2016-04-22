CC=gcc
CFLAGS= -Wall -Wextra

all:
	$(CC) brainfuck.c $(CFLAGS) -o "bf.exe"
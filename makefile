CC?=gcc
CFLAGS=$(FLAGS) -Wall -Wextra -Werror -O3 -std=c99 -pedantic

bin/bf: brainfuck.c
	@echo "Compiling interpreter..."
	@mkdir -p bin
	@$(CC) brainfuck.c $(CFLAGS) -o bin/bf
	@echo "Compilation successful!"

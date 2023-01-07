CC?=gcc
CFLAGS=$(FLAGS) -Wall -Wextra -Werror -std=c99 -pedantic -Iinc -g
SOURCES=$(wildcard src/*.c)
OBJS=$(patsubst src/%.c,objs/%.o,$(SOURCES))
EXECUTABLE=bf

objs/%.o: src/%.c
	@mkdir -p objs
	@echo "Compiling $<..."
	@$(CC) -c $(CFLAGS) -g $< -o $@

bin/$(EXECUTABLE): $(OBJS)
	@echo "Linking interpreter..."
	@mkdir -p bin
	@$(CC) $(OBJS) $(CFLAGS) -o bin/$(EXECUTABLE)
	@echo "Compilation successful!"

clean:
	@echo "Cleaning up binaries..."
	@rm -rf bin objs

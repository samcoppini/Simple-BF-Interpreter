#include <stdlib.h>
#include <stdio.h>

enum CommandKind {
	CMD_CHANGE,
	CMD_SET,
	CMD_MOVE,
	CMD_LOOP_BEGIN,
	CMD_LOOP_END,
	CMD_INPUT,
	CMD_OUTPUT
};

typedef struct Command {
	enum CommandKind type;
	int change_val;
} Command;

typedef struct Commands {
	Command *cmds;
	int num_commands;
} Commands;

typedef struct Node {
	int val;
	struct Node *next;
} Node;

//Reads a file and returns the commands in the file
Commands read_file(FILE *file) {
	Commands commands = {(Command *) malloc(sizeof(Command)), 0};
	Node *loop_stack = NULL;
	int allocated = 1, c;
	while (!feof(file)) {
		switch (c = fgetc(file)) {
			case '+': case '-':
			case '>': case '<': {
				enum CommandKind type = (c == '+' || c == '-') ? CMD_CHANGE: CMD_MOVE;
				int change_amt = (c == '+' || c == '>') ? 1 : -1;
				if (commands.num_commands > 0) {
					if (type == CMD_CHANGE && commands.cmds[commands.num_commands - 1].type == CMD_SET) {
						commands.cmds[commands.num_commands - 1].change_val += change_amt;
						break;
					}
					else if (commands.num_commands > 0 && commands.cmds[commands.num_commands - 1].type == type) {
						commands.cmds[commands.num_commands - 1].change_val += change_amt;
						if (commands.cmds[commands.num_commands - 1].change_val == 0) {
							commands.num_commands--;
						}
						break;
					}
				}
				commands.cmds[commands.num_commands].type = type;
				commands.cmds[commands.num_commands].change_val = change_amt;
				commands.num_commands++;
				break;
			}
				
			case '[': {
				Node *temp = loop_stack;
				loop_stack = (Node *) malloc(sizeof(Node));
				loop_stack->val = commands.num_commands;
				loop_stack->next = temp;
				commands.cmds[commands.num_commands].type = CMD_LOOP_BEGIN;
				commands.num_commands++;
				break;
			}
				
			case ']': {
				int loop_loc = loop_stack->val;
				loop_stack = loop_stack->next;
				if (commands.cmds[commands.num_commands - 1].type == CMD_CHANGE &&
				    commands.cmds[commands.num_commands - 1].change_val % 2 == 1 &&
					commands.cmds[commands.num_commands - 2].type == CMD_LOOP_BEGIN)
				{
					commands.num_commands -= 2;
					commands.cmds[commands.num_commands].type = CMD_SET;
					commands.cmds[commands.num_commands].change_val = 0;
				}
				else {
					commands.cmds[commands.num_commands].type = CMD_LOOP_END;
					commands.cmds[commands.num_commands].change_val = loop_loc;
					commands.cmds[loop_loc].change_val = commands.num_commands;
					commands.num_commands++;
				}
				break;
			}
				
			case '.':
				commands.cmds[commands.num_commands].type = CMD_OUTPUT;
				commands.num_commands++;
				break;
				
			case ',':
				commands.cmds[commands.num_commands].type = CMD_INPUT;
				commands.num_commands++;
				break;
		}
		if (commands.num_commands == allocated) {
			allocated *= 2;
			commands.cmds = (Command *) realloc(commands.cmds, allocated * sizeof(Command));
		}
	}
	fclose(file);
	commands.cmds = (Command *) realloc(commands.cmds, commands.num_commands * sizeof(Command));
	return commands;
}

//Executes a given brainfuck program with the given input file
void execute(Commands commands, FILE *input) {
	char tape[30000] = {0};
	Command *code = commands.cmds;
	int tape_pos, code_pos;
	for (tape_pos = 0, code_pos = 0; code_pos < commands.num_commands; code_pos++) {
		switch (code[code_pos].type) {
			case CMD_CHANGE:
				tape[tape_pos] += code[code_pos].change_val;
				break;
				
			case CMD_SET:
				tape[tape_pos] = code[code_pos].change_val;
				break;
				
			case CMD_MOVE:
				tape_pos += code[code_pos].change_val;
				break;
				
			case CMD_LOOP_BEGIN:
				if (!tape[tape_pos]) {
					code_pos = code[code_pos].change_val;
				}
				break;
				
			case CMD_LOOP_END:
				if (tape[tape_pos]) {
					code_pos = code[code_pos].change_val;
				}
				break;
				
			case CMD_OUTPUT:
				putchar(tape[tape_pos]);
				break;
				
			case CMD_INPUT:
				if (!input) {
					printf("Error, was unable to read input file\n");
					return;
				}
				else
					tape[tape_pos] = feof(input) ? 0: fgetc(input);
				break;
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Error, no brainfuck program given.\n");
		return 0;
	}
	
	FILE *code_file = fopen(argv[1], "r");
	if (!code_file) {
		printf("Error, unable to read the brainfuck program.\n");
		return 0;
	}
	
	Commands code = read_file(code_file);	
	FILE *input_file = argc > 2 ? fopen(argv[2], "r"): NULL;
	execute(code, input_file);
	
	//Clean up everything
	free(code.cmds);
	if (input_file)
		fclose(input_file);

	return 0;
}

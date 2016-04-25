#include <stdlib.h>
#include <stdio.h>

enum CommandKind {
	CMD_CHANGE,
	CMD_ADD_PRODUCT,
	CMD_SET,
	CMD_MOVE,
	CMD_LOOP_BEGIN,
	CMD_LOOP_END,
	CMD_INPUT,
	CMD_OUTPUT
};

typedef struct Command {
	enum CommandKind type;
	int change_val, offset;
} Command;

typedef struct Commands {
	Command *cmds;
	int num_commands;
} Commands;

typedef struct Node {
	int val;
	struct Node *next;
} Node;

//Adds an end to a loop to a list of commands, optimizing it away if possible
void add_loop_end(Commands *commands, int loop_start) {
	//If the previous command is a loop end, we don't need an extra one,
	//so just make sure to adjust the loop beginning
	if (commands->cmds[commands->num_commands - 1].type == CMD_LOOP_BEGIN) {
		commands->cmds[loop_start].change_val = commands->num_commands;
		return;
	}
	
	//Looks through the contents of a loop, and checks to see if it can
	//be optimized, and it keeps track of the leftmost and rightmost cells
	//travelled to relative to the starting position
	int cur_cmd, max_lcell = 0, max_rcell = 0, cur_cell = 0;
	for (cur_cmd = loop_start + 1; cur_cmd != commands->num_commands; cur_cmd++) {
		if (commands->cmds[cur_cmd].type == CMD_MOVE) {
			cur_cell += commands->cmds[cur_cmd].change_val;
			if (cur_cell > max_rcell)
				max_rcell = cur_cell;
			else if (cur_cell < max_lcell)
				max_lcell = cur_cell;
		}
		else if (commands->cmds[cur_cmd].type != CMD_CHANGE)
		//If a loop has commands other than +->< it can't be turned into
		//a series of multiplications, so break out of it
			break;
	}
	
	//If all the commands in the loop were +->< and we end on the same cell we
	//entered the loop on, we are able to change it to a series of multiplications,
	//so we do that
	if (cur_cmd == commands->num_commands && cur_cell == 0) {
		int num_cells = max_rcell - max_lcell + 1;
		int cell_changes[num_cells];
		for (cur_cell = 0; cur_cell < num_cells; cur_cell++) {
			cell_changes[cur_cell] = 0;
		}
		cur_cell = -max_lcell;
		for (cur_cmd = loop_start; cur_cmd != commands->num_commands; cur_cmd++) {
			if (commands->cmds[cur_cmd].type == CMD_CHANGE) {
				cell_changes[cur_cell] += commands->cmds[cur_cmd].change_val;
			}
			else if (commands->cmds[cur_cmd].type == CMD_MOVE) {
				cur_cell += commands->cmds[cur_cmd].change_val;
			}
		}
		//We can't transform the loop to a series of multiplications unless the
		//change to the starting cell is exactly -
		if (cell_changes[-max_lcell] == -1) {
			commands->num_commands = loop_start;
			for (cur_cell = 0; cur_cell < num_cells; cur_cell++) {
				//If the cell changes in the loop, add a multiplication for it
				if (cell_changes[cur_cell] != 0 && cur_cell != -max_lcell) {
					commands->cmds[commands->num_commands].type = CMD_ADD_PRODUCT;
					commands->cmds[commands->num_commands].change_val = cell_changes[cur_cell];
					commands->cmds[commands->num_commands].offset = cur_cell + max_lcell;
					commands->num_commands++;
				}
			}
			//After we've added the multiplications, set the starting cell to 0
			commands->cmds[commands->num_commands].type = CMD_SET;
			commands->cmds[commands->num_commands].change_val = 0;
			commands->num_commands++;
			return;
		}
	}
	
	//If we couldn't optimize the loop, just add a loop end command normally
	commands->cmds[commands->num_commands].type = CMD_LOOP_END;
	commands->cmds[commands->num_commands].change_val = loop_start;
	commands->cmds[loop_start].change_val = commands->num_commands;
	commands->num_commands++;
}

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
				
			case '[':
				if (commands.num_commands == 0 || commands.cmds[commands.num_commands - 1].type == CMD_LOOP_END ||
				   (commands.cmds[commands.num_commands - 1].type == CMD_SET && commands.cmds[commands.num_commands - 1].change_val == 0))
				{
					int bracket_level = 1, c;
					while (bracket_level > 0 && !feof(file)) {
						c = fgetc(file);
						if (c == ']')
							bracket_level--;
						else if (c == '[')
							bracket_level++;
					}
				}
				else {
					Node *temp = loop_stack;
					loop_stack = (Node *) malloc(sizeof(Node));
					loop_stack->val = commands.num_commands;
					loop_stack->next = temp;
					commands.cmds[commands.num_commands].type = CMD_LOOP_BEGIN;
					commands.num_commands++;
				}
				break;
				
			case ']':
				if (loop_stack == NULL) {
					printf("Error! Unmatched ] found in code!");
					free(commands.cmds);
					exit(0);
				}
				else {
					Node *top = loop_stack;
					loop_stack = loop_stack->next;
					add_loop_end(&commands, top->val);
					free(top);
				}
				break;
				
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
	if (loop_stack != NULL) {
		while (loop_stack != NULL) {
			Node *temp = loop_stack;
			loop_stack = loop_stack->next;
			free(temp);
		}
		printf("Error! Unmatched [ found in the code!");
		fclose(file);
		free(commands.cmds);
		exit(0);
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
				
			case CMD_ADD_PRODUCT:
				tape[tape_pos + code[code_pos].offset] += tape[tape_pos] * code[code_pos].change_val;
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

#include <stdlib.h>
#include <stdio.h>

//Reads a file and returns a pointer to a string with that file's contents
char *read_file(FILE *file) {
	int length = 0, allocated = 1;
	unsigned char *str = (unsigned char *) malloc(allocated);
	while (!feof(file)) {
		str[length++] = fgetc(file);
		if (length == allocated) {
			allocated *= 2;
			str = (unsigned char *) realloc(str, allocated);
		}
	}
	fclose(file);
	str[length++] = '\0';
	str = (unsigned char *) realloc(str, length);
	return str;
}

//Executes a given brainfuck program with the given input file
void execute(char *code, FILE *input) {
	char tape[30000] = {0};
	int tape_pos, code_pos;
	for (tape_pos = 0, code_pos = 0; code[code_pos] != '\0'; code_pos++) {
		switch (code[code_pos]) {
			case '+':
				tape[tape_pos]++;
				break;
				
			case '-':
				tape[tape_pos]--;
				break;
				
			case '>':
				tape_pos++;
				break;
				
			case '<':
				tape_pos--;
				break;
				
			case '[':
				if (!tape[tape_pos]) {
					//Move forward to the matching ]
					int bracket_level = 1;
					while (bracket_level > 0) {
						code_pos++;
						if (code[code_pos] == '[')
							bracket_level++;
						else if (code[code_pos] == ']')
							bracket_level--;
					}
				}
				break;
				
			case ']':
				if (tape[tape_pos]) {
					//Move backward to the matching [
					int bracket_level = 1;
					while (bracket_level > 0) {
						code_pos--;
						if (code[code_pos] == ']')
							bracket_level++;
						else if (code[code_pos] == '[')
							bracket_level--;
					}
				}
				break;
				
			case '.':
				putchar(tape[tape_pos]);
				break;
				
			case ',':
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
	
	char *code = read_file(code_file);
	FILE *input_file = argc > 2 ? fopen(argv[2], "r"): NULL;
	execute(code, input_file);
	
	//Clean up everything
	free(code);
	if (input_file)
		fclose(input_file);

	return 0;
}
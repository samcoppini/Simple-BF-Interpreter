#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Whether we should condense loop endings into a single command */
static bool collapse_loop_ends = true;

typedef enum CommandType {
  CMD_CHANGE,
  CMD_ADD_PRODUCT,
  CMD_SET,
  CMD_MOVE,
  CMD_LOOP_BEGIN,
  CMD_LOOP_END,
  CMD_INPUT,
  CMD_OUTPUT
} CommandType;

typedef struct Command {
  enum CommandType type;
  int offset, change_val;
} Command;

/* A struct for keeping track of the BF commands */
typedef struct CommandList {
  Command *cmds;
  int num_cmds, cmds_allocated;
} CommandList;

#define INITIAL_COMMANDS 32

/* A stack for keeping track of loops in the BF code */
typedef struct Stack {
  int *elements;
  int num_elements, num_allocated;
} Stack;

#define INITIAL_STACK_SIZE 16

/* A struct for keeping track of a change to a tape */
typedef struct TapeChange {
  int offset, change;
} TapeChange;

/* A struct for keeping track of all the changes to the tape since the
   last [ ] . , command */
typedef struct Changelist {
  TapeChange *changes;
  int num_changes, num_allocated;
} Changelist;

#define INITIAL_CHANGES 8

/* Returns a pointer to a newly-allocated stack */
Stack *new_stack() {
  Stack *stack = malloc(sizeof(Stack));
  stack->elements = malloc(INITIAL_STACK_SIZE * sizeof(int));
  stack->num_allocated = INITIAL_STACK_SIZE;
  stack->num_elements = 0;
  return stack;
}

/* Frees a stack */
void free_stack(Stack *stack) {
  free(stack->elements);
  free(stack);
}

/* Returns whether a stack is empty */
bool stack_empty(Stack *stack) {
  return stack->num_elements == 0;
}

/* Pushes an integer to to stack, allocating more space if need be */
void stack_push(Stack *stack, int val) {
  if (stack->num_allocated == stack->num_elements) {
    stack->num_allocated <<= 1;
    stack->elements = realloc(stack->elements,
                              stack->num_allocated * sizeof(int));
  }
  stack->elements[stack->num_elements++] = val;
}

/* Pops the stack, returning the popped value */
int stack_pop(Stack *stack) {
  return stack->elements[--stack->num_elements];
}

/* Returns a newly-allocated list of commands */
CommandList *new_command_list() {
  CommandList *list = malloc(sizeof(CommandList));
  list->cmds = malloc(INITIAL_COMMANDS * sizeof(Command));
  list->cmds_allocated = INITIAL_COMMANDS;
  list->num_cmds = 0;
  return list;
}

/* Frees a list of commands */
void free_commands(CommandList *list) {
  free(list->cmds);
  free(list);
}

/* Adds a new command to the end of a command list */
void add_command(CommandList *list, CommandType type, int offset, int val) {
  if (list->cmds_allocated == list->num_cmds) {
    list->cmds_allocated <<= 1;
    list->cmds = realloc(list->cmds, list->cmds_allocated * sizeof(Command));
  }
  list->cmds[list->num_cmds].type = type;
  list->cmds[list->num_cmds].offset = offset;
  list->cmds[list->num_cmds].change_val = val;
  list->num_cmds++;
}

/* Returns a pointer to a new list of changes */
Changelist *new_changelist() {
  Changelist *cl = malloc(sizeof(Changelist));
  cl->changes = malloc(INITIAL_CHANGES * sizeof(TapeChange));
  cl->num_allocated = INITIAL_CHANGES;
  cl->num_changes = 0;
  return cl;
}

/* Adds a change to a list of changes. If a change already exists at a given
   offset, it adds to that, otherwise it adds a new change */
void add_change(Changelist *cl, int offset, int change) {
  int i;

  for (i = 0; i < cl->num_changes; i++) {
    if (cl->changes[i].offset == offset) {
      cl->changes[i].change += change;
      return;
    }
  }

  if (cl->num_allocated == cl->num_changes) {
    cl->num_allocated <<= 1;
    cl->changes = realloc(cl->changes, cl->num_allocated * sizeof(TapeChange));
  }

  cl->changes[cl->num_changes].offset = offset;
  cl->changes[cl->num_changes].change = change;
  cl->num_changes++;
}

/* Frees a list of changes */
void free_changes(Changelist *cl) {
  free(cl->changes);
  free(cl);
}

/* Adds the list of changes to a list of commands */
void push_changes_to_commands(CommandList *list, Changelist *cl, int pos) {
  int i;

  /* Goes through every change, adding it to the commands if it is nonzero */
  for (i = 0; i < cl->num_changes; i++) {
    if (cl->changes[i].change != 0) {
      add_command(list, CMD_CHANGE, cl->changes[i].offset,
                                    cl->changes[i].change);
    }
  }

  /* Move the tape pointer to the appropriate place after the changes have
     been made */
  if (pos != 0)
    add_command(list, CMD_MOVE, 0, pos);
  cl->num_changes = 0;
}

/* Adds an end to a loop, optimizing it away if possible */
void add_loop_end(CommandList *list, int loop_start) {
  /* If the previous command is a loop end, we don't need an extra one,
     so we just make sure to adjust the loop's beginning */
  if (list->cmds[list->num_cmds - 1].type == CMD_LOOP_END
      && collapse_loop_ends)
  {
		list->cmds[loop_start].change_val = list->num_cmds - 1;
		return;
	}

  bool can_opt = true;
  int cur_cmd, start_cell_change = 0;
  /* Look through the commands in a loop to see if it can be optimized into a
     series of multiplications. This can only occur when the loop is made
     only of CMD_CHANGEs, and the change to the current cell is exactly -1 */
  for (cur_cmd = loop_start + 1; cur_cmd < list->num_cmds; cur_cmd++) {
    if (list->cmds[cur_cmd].type == CMD_CHANGE) {
      if (list->cmds[cur_cmd].offset == 0)
        start_cell_change = list->cmds[cur_cmd].change_val;
    }
    else {
      can_opt = false;
      break;
    }
  }
  if (can_opt && start_cell_change == -1) {
    int copy_offset = 1;
    /* Go through the loop, converting CMD_CHANGE to CMD_ADD_PRODUCT, and
       removing the change to the initial cell */
    for (cur_cmd = loop_start; cur_cmd + copy_offset < list->num_cmds; cur_cmd++) {
      if (list->cmds[cur_cmd + copy_offset].offset == 0) {
        copy_offset++;
        cur_cmd--;
        continue;
      }
      list->cmds[cur_cmd] = list->cmds[cur_cmd + copy_offset];
      list->cmds[cur_cmd].type = CMD_ADD_PRODUCT;
    }
    list->num_cmds -= copy_offset;
    /* Set the initial cell to 0 after the loop */
    add_command(list, CMD_SET, 0, 0);
    return;
  }

  /* If we were unable to optimize the loop, add the loop end normally, and
     adjust the beginning of the loop so it points to the new loop ending */
  list->cmds[loop_start].change_val = list->num_cmds;
  add_command(list, CMD_LOOP_END, 0, loop_start);
}

/* Reads a brainfuck file, returning a list of commands */
CommandList *read_file(FILE *file) {
  CommandList *list = new_command_list();
  Stack *loop_stack = new_stack();
  Changelist *changes = new_changelist();
  int c, cur_pos = 0;
  while ((c = fgetc(file)) != EOF) {
    switch (c) {
      case '-':
        add_change(changes, cur_pos, -1);
        break;

      case '+':
        add_change(changes, cur_pos, 1);
        break;

      case '<':
        cur_pos--;
        break;

      case '>':
        cur_pos++;
        break;

      case '[':
        push_changes_to_commands(list, changes, cur_pos);
        cur_pos = 0;
        /* If we know that this loop can never be entered (if it's before any
           other command, or it immediately follows another loop), we remove
           the contents of the loop entirely */
        if (list->num_cmds == 0 ||
            list->cmds[list->num_cmds - 1].type == CMD_LOOP_END ||
            (list->cmds[list->num_cmds - 1].type == CMD_SET &&
             list->cmds[list->num_cmds - 1].change_val == 0))
        {
          int loop_level = 1;
          while (loop_level > 0 && (c = fgetc(file)) != EOF) {
            if (c == '[')
              loop_level++;
            else if (c == ']')
              loop_level--;
          }
          if (loop_level > 0) {
            fprintf(stderr, "Error! Unmatched '[' found in the code!\n");
            free_commands(list);
            exit(1);
          }
        }
        /* If we couldn't remove the loop, we add the loop start to the command
           list and push its location onto the loop stack so we can complete
           the loop when we encounter the matching ] command*/
        else {
          add_command(list, CMD_LOOP_BEGIN, 0, 0);
          stack_push(loop_stack, list->num_cmds - 1);
        }
        break;

      case ']':
        if (stack_empty(loop_stack)) {
          fprintf(stderr, "Error! Unmatched ']' found in the code.\n");
          free_commands(list);
          exit(1);
        }
        else {
          push_changes_to_commands(list, changes, cur_pos);
          cur_pos = 0;
          add_loop_end(list, stack_pop(loop_stack));
        }
        break;

      case '.':
        push_changes_to_commands(list, changes, cur_pos);
        cur_pos = 0;
        add_command(list, CMD_OUTPUT, 0, 0);
        break;

      case ',':
        push_changes_to_commands(list, changes, cur_pos);
        cur_pos = 0;
        add_command(list, CMD_INPUT, 0, 0);
        break;
    }
  }
  if (!stack_empty(loop_stack)) {
    fprintf(stderr, "Error! Unmatched '[' found in the code.\n");
    free_commands(list);
    exit(1);
  }
  fclose(file);
  free_stack(loop_stack);
  free_changes(changes);
  return list;
}

/* Executes a given brainfuck program with a given input file */
void execute(CommandList *list, FILE *input_file) {
  char tape[30000] = {0};
  Command *code = list->cmds;
  int tape_pos, code_pos, c;
  for (tape_pos = 0, code_pos = 0; code_pos < list->num_cmds; code_pos++) {
    switch (code[code_pos].type) {
      case CMD_CHANGE:
        tape[tape_pos + code[code_pos].offset] += code[code_pos].change_val;
        break;

      case CMD_ADD_PRODUCT: {
        int product = tape[tape_pos] * code[code_pos].change_val;
        tape[tape_pos + code[code_pos].offset] += product;
        break;
      }

      case CMD_SET:
        tape[tape_pos] = code[code_pos].change_val;
        break;

      case CMD_MOVE:
        tape_pos += code[code_pos].change_val;
        break;

      case CMD_LOOP_BEGIN:
        if (tape[tape_pos] == 0) {
          code_pos = code[code_pos].change_val;
        }
        break;

      case CMD_LOOP_END:
        if (tape[tape_pos] != 0) {
          code_pos = code[code_pos].change_val;
        }
        break;

      case CMD_OUTPUT:
        putchar(tape[tape_pos]);
        break;

      case CMD_INPUT:
        if (!input_file) {
          fprintf(stderr, "Error! No input file given!\n");
          return;
        }
        c = fgetc(input_file);
        tape[tape_pos] = (c == EOF ? 0: c);
        break;
    }
  }
}

/* Outputs the equivalent C source code for the brainfuck program */
void compile(CommandList *list) {
  int i, j, tab;

  printf("#include <stdio.h>\n\n"
         "int main(int argc, char *argv[]) {\n"
         "   char tape[30000] = {0};\n"
         "   char *tp = tape;\n"
         "   FILE *fp = (argc > 1) ? fopen(argv[1], \"r\"): stdin;\n"
         "   if (!fp) { fprintf(stderr, \"Error opening input!\"); return 1; }"
         "\n   int c;");

  for (i = 0, tab = 1; i < list->num_cmds; i++) {
    putchar('\n');
    if (list->cmds[i].type == CMD_LOOP_END)
      tab--;
    for (j = 0; j < tab * 3; j++) {
      putchar(' ');
    }
    switch (list->cmds[i].type) {
      case CMD_CHANGE:
        printf("tp[%d] += %d;", list->cmds[i].offset,
                                list->cmds[i].change_val);
        break;

      case CMD_ADD_PRODUCT:
        printf("tp[%d] += *tp * %d;", list->cmds[i].offset,
                                      list->cmds[i].change_val);
        break;

      case CMD_SET:
        printf("*tp = %d;", list->cmds[i].change_val);
        break;

      case CMD_MOVE:
        printf("tp += %d;", list->cmds[i].change_val);
        break;

      case CMD_LOOP_BEGIN:
        printf("while (*tp) {");
        tab++;
        break;

      case CMD_LOOP_END:
        printf("}");
        break;

      case CMD_OUTPUT:
        printf("putchar(*tp);");
        break;

      case CMD_INPUT:
        printf("*tp = ((c = fgetc(fp)) == EOF) ? 0: c;");
        break;
    }
  }
  printf("\n}");
}

int main(int argc, char *argv[]) {
  char *bf_filename = NULL, *input_filename = NULL;
  bool compile_bf = false;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "-i") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Error! %s option requires an argument\n", argv[i -1]);
        return 1;
      }
      else {
        input_filename = argv[i];
      }
    }
    else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0) {
      compile_bf = true;
      /* Loop ends aren't collapsed when compiling to C, because it would be
         more difficult to translate and would provide no benefit anyway */
      collapse_loop_ends = false;
    }
    else {
      if (bf_filename == NULL) {
        bf_filename = argv[i];
      }
      else {
        fprintf(stderr, "Error! Can only read one BF file at a time!");
        return 1;
      }
    }
  }

  if (bf_filename == NULL) {
    fprintf(stderr, "Error! No BF file provided!\n");
    return 1;
  }

  FILE *bf_file = fopen(bf_filename, "r");
  if (!bf_file) {
    fprintf(stderr, "Unable to open '%s'.", bf_filename);
    return 1;
  }

  FILE *input_file = NULL;
  if (input_filename == NULL)
    input_file = stdin;
  else {
    input_file = fopen(input_filename, "r");
    if (!input_file) {
      fprintf(stderr, "Error! Unable to open '%s'.\n", input_filename);
      return 1;
    }
  }

  CommandList *commands = read_file(bf_file);
  if (compile_bf)
    compile(commands);
  else
    execute(commands, input_file);

  free_commands(commands);

  return 0;
}

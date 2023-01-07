//
//  Copyright 2023 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "bytecode.h"

#include <stdlib.h>

struct AddressStack {
    uint32_t *addresses;

    size_t alloc, len;
};

#define INIT_ADDRESS_ALLOC 16

struct AddressStack create_address_stack(void) {
    struct AddressStack stack;
    stack.addresses = malloc(sizeof(uint32_t) * INIT_ADDRESS_ALLOC);
    stack.alloc = INIT_ADDRESS_ALLOC;
    stack.len = 0;
    return stack;
}

void stack_add(struct AddressStack *stack, uint32_t address) {
    if (stack->alloc == stack->len) {
        stack->alloc *= 2;
        stack->addresses = realloc(stack->addresses, sizeof(uint32_t) * stack->alloc);
    }
    stack->addresses[stack->len++] = address;
}

uint32_t stack_pop(struct AddressStack *stack) {
    return stack->addresses[--stack->len];
}

struct Bytecode {
    uint8_t *instructions;

    size_t alloc, len;
};

#define INIT_BYTECODE_ALLOC 256

struct Bytecode create_bytecode(void) {
    struct Bytecode bytecode;
    bytecode.instructions = malloc(INIT_BYTECODE_ALLOC);
    bytecode.alloc = INIT_BYTECODE_ALLOC;
    bytecode.len = 0;
    return bytecode;
}

void bytecode_add(struct Bytecode *bytecode, uint8_t byte) {
    if (bytecode->alloc == bytecode->len) {
        bytecode->alloc *= 2;
        bytecode->instructions = realloc(bytecode->instructions, bytecode->alloc);
    }
    bytecode->instructions[bytecode->len++] = byte;
}

void add_placeholder_address(struct Bytecode *bytecode) {
    for (int i = 0; i < 4; i++) {
        bytecode_add(bytecode, 0);
    }
}

void replace_address(struct Bytecode *bytecode, uint32_t index, uint32_t new_address) {
    bytecode->instructions[index - 4] = (new_address & 0xFF000000) >> 24;
    bytecode->instructions[index - 3] = (new_address & 0x00FF0000) >> 16;
    bytecode->instructions[index - 2] = (new_address & 0x0000FF00) >> 8;
    bytecode->instructions[index - 1] = (new_address & 0x000000FF) >> 0;
}

uint8_t *get_bf_bytecode(FILE *file) {
    struct Bytecode bytecode = create_bytecode();
    struct AddressStack stack = create_address_stack();
    int c;

    while ((c = fgetc(file)) != EOF) {
        switch (c) {
            case '+':
                bytecode_add(&bytecode, BF_INC);
                break;

            case '-':
                bytecode_add(&bytecode, BF_DEC);
                break;

            case '<':
                bytecode_add(&bytecode, BF_LEFT);
                break;

            case '>':
                bytecode_add(&bytecode, BF_RIGHT);
                break;

            case '.':
                bytecode_add(&bytecode, BF_OUTPUT);
                break;

            case ',':
                bytecode_add(&bytecode, BF_INPUT);
                break;

            case '[':
                bytecode_add(&bytecode, BF_LOOP_START);
                add_placeholder_address(&bytecode);
                stack_add(&stack, bytecode.len);
                break;

            case ']': {
                if (stack.len == 0) {
                    fprintf(stderr, "Unmatched ]\n");
                    goto err;
                }
                bytecode_add(&bytecode, BF_LOOP_END);
                add_placeholder_address(&bytecode);
                uint32_t loop_start = stack_pop(&stack);
                replace_address(&bytecode, bytecode.len, loop_start);
                replace_address(&bytecode, loop_start, bytecode.len);
                break;
            }
        }
    }

    if (stack.len > 0) {
        fprintf(stderr, "Unmatched [\n");
        goto err;
    }

    bytecode_add(&bytecode, BF_QUIT);
    free(stack.addresses);

    return bytecode.instructions;

err:
    free(bytecode.instructions);
    free(stack.addresses);
    return NULL;
}

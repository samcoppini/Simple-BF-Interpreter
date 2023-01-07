//
//  Copyright 2023 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "bytecode.h"
#include "interpreter.h"

#define TAPE_LENGTH 30000

uint32_t read_address(const uint8_t *code_ptr) {
    uint32_t address = 0;
    address = (address << 8) | *code_ptr++;
    address = (address << 8) | *code_ptr++;
    address = (address << 8) | *code_ptr++;
    address = (address << 8) | *code_ptr++;
    return address;
}

bool interpret_bf_bytecode(const uint8_t *bytecode) {
    const uint8_t *code_ptr = bytecode;
    uint8_t tape[TAPE_LENGTH] = { 0 };
    int tape_index = 0;

    while (true) {
        switch (*code_ptr++) {
            case BF_INC:
                tape[tape_index]++;
                break;

            case BF_DEC:
                tape[tape_index]--;
                break;

            case BF_ADD:
                tape[tape_index] += *code_ptr++;
                break;

            case BF_LEFT:
                if (tape_index == 0) {
                    fprintf(stderr, "Moved too far left!\n");
                    return false;
                }
                tape_index--;
                break;

            case BF_RIGHT:
                tape_index++;
                if (tape_index == TAPE_LENGTH) {
                    fprintf(stderr, "Moved too far right!\n");
                    return false;
                }
                break;

            case BF_MOVE:
                tape_index += (int8_t) *code_ptr++;
                break;

            case BF_OUTPUT:
                putchar(tape[tape_index]);
                break;

            case BF_INPUT: {
                int c = getchar();
                tape[tape_index] = (c == EOF ? 0 : c);
                break;
            }

            case BF_LOOP_START:
                if (!tape[tape_index]) {
                    code_ptr = &bytecode[read_address(code_ptr)];
                }
                else {
                    code_ptr += 4;
                }
                break;

            case BF_LOOP_END:
                if (tape[tape_index]) {
                    code_ptr = &bytecode[read_address(code_ptr)];
                }
                else {
                    code_ptr += 4;
                }
                break;

            case BF_QUIT:
                return true;

            default:
                break;
        }
    }
}

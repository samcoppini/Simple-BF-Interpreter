//
//  Copyright 2023 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "bytecode.h"
#include "interpreter.h"

#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <brainfuck-file.bf>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Unable to open %s\n", argv[1]);
        return 2;
    }

    uint8_t *bytecode = get_bf_bytecode(file);
    fclose(file);

    if (bytecode == NULL) {
        return 3;
    }

    int successful = interpret_bf_bytecode(bytecode);
    free(bytecode);

    return successful ? 0 : 1;
}

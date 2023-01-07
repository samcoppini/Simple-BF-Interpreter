//
//  Copyright 2023 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdio.h>
#include <stdint.h>

enum BfOpCode {
    BF_INC,
    BF_DEC,
    BF_ADD,
    BF_LEFT,
    BF_RIGHT,
    BF_INPUT,
    BF_OUTPUT,
    BF_LOOP_START,
    BF_LOOP_END,
    BF_QUIT,
};

uint8_t *get_bf_bytecode(FILE *file);

#endif // BYTECODE_H

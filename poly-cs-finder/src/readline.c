#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "readline.h"


uint32_t readline(FILE* stream, uint32_t length, char line[length]) {
    assert(stream != NULL);
    assert(line != NULL);
    assert(length != 0);

    uint32_t read = 0;
    int character = EOF;
    while (read < length - 1) {
        character = getc(stream);

        if (character == EOF) {
            break; 
        }

        line[read] = character;
        read++;

        if (character == '\n') {
            break;
        }
    }
    line[read] = '\0';

    assert(read <= length);
    return read;
}

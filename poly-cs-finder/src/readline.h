#ifndef READ_LINE_H_
#define READ_LINE_H_

#include <stdio.h>
#include <stdint.h>

uint32_t readline(FILE* stream, uint32_t length, char line[length]);

#endif /* READ_LINE_H_ */

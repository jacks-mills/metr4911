#ifndef ERROR_H_
#define ERROR_H_

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OKAY  = 0,
    ERROR = 1
} error_t;

#define exit_on_error(error, exitStatus, ...) \
    do {if(error) {fprintf(stderr, __VA_ARGS__); exit(exitStatus);}} while (0)


#endif /* ERROR_H_ */

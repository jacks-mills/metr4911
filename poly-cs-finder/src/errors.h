#ifndef ERRORS_H_
#define ERRORS_H_

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OKAY = 0,
    ERROR
} error_t;


#define exit_on_error(error, exitStatus, ...) \
    if(error) {fprintf(stderr, __VA_ARGS__); exit(exitStatus);}


#endif /* ERRORS_H_ */

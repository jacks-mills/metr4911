#ifndef COVER_H_
#define COVER_H_

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>
#include "errors.h"


#define COVER_MAX_ORDER     8
#define COVER_MAX_POLYOMINOES   COVER_MAX_ORDER

/* Derived from there being N^2 cells in a cover, meaning N^2 alpha symbols in a string
 * representation. There are additionally N-1 spaces between each row of symbols, and +1 for the
 * terminating null character */
#define COVER_MAX_COVER_STRING_LENGTH   COVER_MAX_ORDER * COVER_MAX_ORDER + COVER_MAX_ORDER
#define COVER_MAX_NUM_PARTIAL_COVERS    (1 << COVER_MAX_ORDER)



struct Cover {
    uint64_t polys[COVER_MAX_POLYOMINOES];
    uint32_t numPolys;
    uint32_t order;
};

void    cover_print(const struct Cover *cover);
error_t cover_from_string(
            uint64_t stringLength, const char string[stringLength],
            struct Cover *cover);
error_t cover_status_update(
            FILE *universe, 
            uint32_t length, bool status[length],
            const struct Cover *cover);

#endif /* COVER_H_ */

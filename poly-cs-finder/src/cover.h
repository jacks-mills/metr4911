#ifndef COVER_H_
#define COVER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdint.h>
#include "errors.h"


#define COVER_MAX_ORDER     8
#define COVER_MAX_POLYOMINOES   COVER_MAX_ORDER

/* Derived from there being N^2 cells in a cover, meaning N^2 alpha symbols in a string
 * representation. There are additionally N-1 spaces between each row of symbols, and +1 for the
 * terminating null character */
#define COVER_MAX_COVER_STRING_LENGTH   COVER_MAX_ORDER * COVER_MAX_ORDER + COVER_MAX_ORDER
#define COVER_MAX_NUM_PARTIAL_COVERS    (1 << COVER_MAX_ORDER)

#define STATUS_UNDEFINED    0
#define STATUS_SET          1
#define STATUS_DEFINING_SET 2
#define STATUS_CRITICAL_SET 3



struct Cover {
    uint64_t polys[COVER_MAX_POLYOMINOES];
    uint32_t numPolys;
    uint32_t order;
};

void    cover_print(const struct Cover *cover);
void    cover_print_string_from_status(
        FILE *stream,
        uint32_t length, const uint8_t status[length],
        const struct Cover *cover);

error_t cover_from_string(uint64_t length, const char string[length], struct Cover *cover);
void    cover_to_string(uint32_t length, char string[length], const struct Cover *cover);

error_t cover_status_mark_intersections(
            uint32_t length, uint8_t status[length],
            FILE *universe, const struct Cover *cover);
error_t cover_find_supersets(
            FILE* universe, const struct Cover *cover, uint32_t mask,
            uint32_t *length, struct Cover supersets[*length]);
/* Read every cover in a universe file into a newly allocated array (free with free()). */
error_t cover_universe_load(FILE *universe, uint32_t *length, struct Cover **covers);

void cover_status_mark_subsets(uint32_t length, uint8_t status[length], const struct Cover *cover);
void cover_status_mark_supersets(uint32_t length, uint8_t status[length], const struct Cover *cover);

#endif /* COVER_H_ */

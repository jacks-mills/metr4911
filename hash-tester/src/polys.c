#include <stdint.h>
#include <assert.h>
#include <stddef.h>

#include "error.h"
#include "polys.h"


#define IS_TILE(chr) ('A' <= chr && chr <= 'Z')
#define IS_SEPARATOR(chr) (chr == ' ')

static uint64_t *poly_get(
        uint32_t *polysCount, uint32_t polysLength, uint64_t polys[polysLength],
        char tile, uint64_t *tile2poly[26]) {
    assert(polysCount != NULL);
    assert(polys != NULL);
    assert(tile2poly != NULL);
    assert(IS_TILE(tile));

    uint64_t *ret = NULL;

    assert(0 <= tile - 'A' && tile - 'A' <= 25);
    ret = tile2poly[tile - 'A'];
    if (ret != NULL) {
        return ret;
    }

    if (*polysCount >= polysLength) {
        return NULL;
    }
    ret = tile2poly[tile - 'A'] = &polys[*polysCount];
    (*polysCount)++;
    return ret;
}

error_t polys_from_string(
        uint32_t *polysCount, uint32_t polysLength, uint64_t polys[polysLength],
        uint32_t stringLength, const char string[stringLength]) {
    assert(string != NULL);
    assert(polysCount != NULL);
    assert(polys != NULL);

    uint64_t *tile2poly[26] = { NULL };
    uint32_t tileIndex = 0;
    uint32_t stringIndex = 0;
    char chr = '\0';
    while (stringIndex < stringLength) {
        chr = string[stringIndex];
        stringIndex++;

        if (IS_TILE(chr)) {
            uint64_t *poly = poly_get(polysCount, polysLength, polys, chr, tile2poly);
            if (poly == NULL) {
                return ERROR;
            }

            if (tileIndex >= 64) {
                return ERROR;
            }

            *poly |= (1UL << tileIndex);
            tileIndex++;

            continue;
        }

        if (IS_SEPARATOR(chr)) {
            continue;
        }

        return ERROR;
    }

    return OKAY;
}

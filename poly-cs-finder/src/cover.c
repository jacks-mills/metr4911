#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "readline.h"
#include "errors.h"
#include "cover.h"

#define IS_SYMBOL(chr)          (('a' <= chr && chr <= 'z') || ('A' <= chr && chr <= 'Z'))
#define IS_SEPARATOR(chr)       (chr == ' ')
#define IS_COMMENT_PREFIX(chr)  (chr == '#')


void cover_print(const struct Cover *cover) {
    assert(cover != NULL);
    printf("Cover (%p)\n", (void*) cover);
    printf("order: %i num polyominoes: %i\n", cover->order, cover->numPolys);

    uint32_t polysLength = sizeof(cover->polys)/sizeof(*cover->polys);
    uint32_t polysNumber = cover->numPolys;
    uint32_t polysUpperBound = (polysLength < polysNumber) ? polysLength : polysNumber;

    uint32_t digits = 0;
    for (uint32_t i = polysUpperBound; i > 0; i /= 10) {
        digits++;
    }

    for (uint32_t i = 0; i < polysUpperBound; i++) {
        printf("%*u: ", digits, i);

        for (uint32_t j = 0; j < (cover->order * cover->order); j++) {
            printf("%c", (cover->polys[i] & (1UL << j)) ? '1' : '0');

        }

        printf(" %lu", cover->polys[i]);
        printf("\n");
    }
}

void cover_to_string( uint32_t length, char string[length], const struct Cover *cover) {
    char poly2symbol[52 + 1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.";
    assert(cover->numPolys + 1 <= sizeof(poly2symbol));
    poly2symbol[cover->numPolys] = '.'; /* assigin '.' to no poly */

    uint32_t stringIndex = 0;
    uint32_t cellIndex = 0;
    uint32_t polyIndex = 0;
    assert((cover->order + 1) * cover->order <= length);
    while (cellIndex < cover->order * cover->order) {
        if (cellIndex != 0 && cellIndex % cover->order == 0) {
            string[stringIndex] = ' ';
            stringIndex++;
        }

        for (polyIndex = 0; polyIndex < cover->numPolys; polyIndex++) {
            if (cover->polys[polyIndex] & (1UL << cellIndex)) {
                break;
            }
        }
        string[stringIndex] = poly2symbol[polyIndex];
        stringIndex++;
        
        cellIndex++;
    }

    string[stringIndex] = '\0';
}

static void cover_to_string_masked(
        uint32_t length, char string[length],
        const struct Cover *cover, uint32_t mask) {
    char poly2symbol[52 + 1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.";
    assert(cover->numPolys + 1 <= sizeof(poly2symbol));
    poly2symbol[cover->numPolys] = '.'; /* assigin '.' to no poly */

    uint32_t stringIndex = 0;
    uint32_t cellIndex = 0;
    uint32_t polyIndex = 0;
    assert((cover->order + 1) * cover->order <= length);
    while (cellIndex < cover->order * cover->order) {
        if (cellIndex != 0 && cellIndex % cover->order == 0) {
            string[stringIndex] = ' ';
            stringIndex++;
        }

        for (polyIndex = 0; polyIndex < cover->numPolys; polyIndex++) {
            if (cover->polys[polyIndex] & (1UL << cellIndex) && mask & (1UL << polyIndex)) {
                break;
            }
        }
        string[stringIndex] = poly2symbol[polyIndex];
        stringIndex++;
        
        cellIndex++;
    }

    string[stringIndex] = '\0';
}

void cover_print_string_from_status(
         uint32_t length, const uint8_t status[length],
         const struct Cover *cover) {
    for (uint32_t i = 0; i < length; i++) {
        if (!status[i]) {
            continue;
        }
        
        char coverString[COVER_MAX_COVER_STRING_LENGTH] = {0};
        cover_to_string_masked(sizeof(coverString), coverString, cover, i);
        printf("%s\n", coverString);
    }
}

error_t cover_from_string(
            uint64_t stringLength, const char string[stringLength],
            struct Cover *cover) {
    uint64_t *symbol2subset[52] = { NULL };
    uint32_t symbolCount[52] = { 0 };

    memset(cover->polys, 0, sizeof(cover->polys));
    cover->numPolys = 0;
    cover->order = 0;

    bool orderIsSet = false;
    bool lastWasSeparator = false;

    uint32_t symbolsSeen = 0;
    uint32_t symbolIndex = 0;
    uint32_t stringIndex = 0;
    uint32_t columns = 0;
    uint32_t rows = 1;

    /* the first character should be a symbol */
    if (!IS_SYMBOL(string[0])) {
        return ERROR;
    }

    while(stringIndex < stringLength) {
        char symbol = string[stringIndex];

        if (!IS_SYMBOL(symbol)) {
            if (!IS_SEPARATOR(symbol)) {
                return ERROR;
            }

            if (lastWasSeparator) {
                return ERROR;
            }
            lastWasSeparator = true;

            if (!orderIsSet) {
                orderIsSet = true;
                cover->order = columns; 
            }

            if (columns != cover->order) {
                return ERROR;
            }
            columns = 0;
            rows++;

            stringIndex++;
            continue;
        }

        lastWasSeparator = false;
        columns++;

        symbolIndex = ('A' <= symbol && symbol <= 'Z') ? symbol - 'A' : symbol - 'a' + 26;

        if (symbol2subset[symbolIndex] == NULL) {
            if (cover->numPolys >= COVER_MAX_POLYOMINOES) {
                return ERROR;
            }
            symbol2subset[symbolIndex] = &cover->polys[cover->numPolys];
            cover->numPolys++;
        }
        *symbol2subset[symbolIndex] |= 1UL << symbolsSeen;

        symbolCount[symbolIndex]++;
        if (orderIsSet && symbolCount[symbolIndex] > cover->order) {
            return ERROR;
        }

        symbolsSeen++;
        stringIndex++;
    }

    if (orderIsSet && (columns != cover->order || rows != cover->order)) {
        return ERROR;
    }

    return OKAY;
}

static int getc_and_incr(FILE *stream, uint32_t *read) {
    /* stream and read cannot be NULL, but choosing not to assert it */
    assert(*read != UINT32_MAX); /* protect against overflow */
    int character = getc(stream);
    (*read)++;
    return character;
}

static error_t universe_process_line_cover_sequence(
            FILE *universe, int *character, uint32_t *read,
            struct Cover *cover) {
    assert(universe != NULL);
    assert(character != NULL);
    assert(read != NULL);
    
    char coverString[COVER_MAX_COVER_STRING_LENGTH] = {0};
    uint32_t coverStringIndex = 0;

    while (coverStringIndex < sizeof(coverString) - 1
                && *read != UINT32_MAX
                && (IS_SYMBOL(*character) || IS_SEPARATOR(*character))) {
        coverString[coverStringIndex] = *character;
        *character = getc_and_incr(universe, read);

        if (IS_SEPARATOR(*character) && IS_SEPARATOR(coverString[coverStringIndex])) {
            break;
        }
        coverStringIndex++;
    }
    coverString[coverStringIndex] = '\0';

    if (*read == UINT32_MAX) {
        return ERROR;
    }

    if (cover != NULL) {
        assert(coverStringIndex < sizeof(coverString));
        return cover_from_string(coverStringIndex, coverString, cover);
    }
    return OKAY;
}

enum LineStatus {
    LS_EOF = 0,
    LS_COMMENT,
    LS_COVER,
    LS_EMPTY,
    LS_BAD,
};

static enum LineStatus universe_process_line(FILE *universe, struct Cover *cover) {
    assert(universe != NULL);

    uint32_t read = 0;
    int character = getc_and_incr(universe, &read);
    if (character == EOF) {
        return LS_EOF;
    }

    enum LineStatus ret = LS_EMPTY;

    bool isComment = false;
    bool seenCover = false;
    while(character != '\n' && character != EOF && read != UINT32_MAX) {
        if (isComment) {
            character = getc_and_incr(universe, &read);
            continue;
        }

        if (isblank(character)) {
            character = getc_and_incr(universe, &read);
            continue;
        }

        if (IS_COMMENT_PREFIX(character)) {
            isComment = true;
            ret = LS_COMMENT;
            continue;
        }

        if (IS_SYMBOL(character) && !seenCover) {
            seenCover = true;
            ret = LS_COVER;
            error_t error = OKAY;
            error = universe_process_line_cover_sequence(universe, &character, &read, cover);
            if (error) {
                ret = LS_BAD;
                break;
            }
            continue;
        }

        /* unexpected symbol */
        ret = LS_BAD;
        break;
    }

    if (read == UINT32_MAX) {
        return LS_BAD;
    }

    return ret;
}

static uint32_t cover_intersection_get(
            const struct Cover *primary, const struct Cover *secondary) {
    assert(primary != NULL);
    assert(secondary != NULL);
    uint32_t intersection = 0;
    for (uint32_t i = 0; i < primary->numPolys; i++) {
        for (uint32_t j = 0; j < secondary->numPolys; j++) {
            if (primary->polys[i] == secondary->polys[j]) {
                intersection |= (1UL << i); 
                break;
            }
        }
    }
    return intersection;
}

error_t cover_status_mark_intersections(
            uint32_t length, uint8_t status[length],
            FILE *universe, const struct Cover *cover) {
    assert(universe != NULL);
    assert(cover != NULL);
    assert(length >= (1UL << cover->order));

    error_t ret = OKAY;
    struct Cover universeCover = {0};
    struct Cover *coverp = &universeCover;

    bool universeIsValid = true;

    uint32_t lineNumber = 1;
    enum LineStatus lineStatus = universe_process_line(universe, coverp);
    while (lineStatus != LS_EOF && lineNumber != UINT32_MAX) {
        switch (lineStatus) {
        default:
            break;
        case LS_BAD:
            ret = ERROR;
            coverp = NULL;
            universeIsValid = false;
            printf("%u: Bad line\n", lineNumber);
            break;
        case LS_COVER:
            if (!universeIsValid) {
                break;
            }
            uint32_t intersection = cover_intersection_get(cover, coverp);
            assert(intersection < length);
            status[intersection] = false;
            break;
        }

        lineStatus = universe_process_line(universe, coverp);
        lineNumber++;
    }

    if (lineNumber == UINT32_MAX) {
        return ERROR;
    }

    if (universeIsValid) {
        status[(1UL << cover->order) - 1] = true;
    }

    return ret;
}

void cover_status_mark_subsets(uint32_t length, uint8_t status[length], const struct Cover *cover) {
    for (uint32_t partialCover = length - 1; partialCover > 0; partialCover--) {
        if (status[partialCover]) {
            continue;
        }

        /* mark subsets one level down */
        for (uint32_t polyIndex = 0; polyIndex < cover->numPolys; polyIndex++) {
            if (!(partialCover & (1UL << polyIndex))) {
                continue;
            }
            status[partialCover & ~(1UL << polyIndex)] = false;
        }
            
    }
}

void cover_status_mark_supersets(uint32_t length, uint8_t status[length], const struct Cover *cover) {
    for (uint32_t partialCover = 0; partialCover < length - 1; partialCover++) {
        if (!status[partialCover]) {
            continue;
        }

        /* mark subsets one level up */
        for (uint32_t polyIndex = 0; polyIndex < cover->numPolys; polyIndex++) {
            if (partialCover & (1UL << polyIndex)) {
                continue;
            }
            status[partialCover | (1UL << polyIndex)] = false;
        }
    }
}

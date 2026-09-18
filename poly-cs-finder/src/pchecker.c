#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>


#include "errors.h"
#include "cover.h"

#define TEXT_COLOUR_FG_RED      "\033[31m"
#define TEXT_COLOUR_FG_GREEN    "\033[32m"
#define TEXT_COLOUR_FG_RESET    "\033[39m"
#define TEXT_RESET              "\033[0m"

#define EXIT_IS_CRITICAL        0
#define EXIT_IS_NOT_CRITICAL    1
#define EXIT_ERROR              2

void usage(void) {
    const char *programName = "pchecker";
    fprintf(stderr, "%s [-v] COVER UNIVERSE\n", programName);
    exit(EXIT_ERROR);
}

int main(int argc, char *argv[argc]) {
    error_t error = OKAY;
    bool verbose = false;

    if (argc != 3 && argc != 4) usage();

    if (argc == 4) {
        if (strcmp(argv[1], "-v") != 0) usage();
        verbose = true;
        argv++;
        argc--;
    }

    struct Cover cover = {0};
    error = cover_from_string(strlen(argv[1]), argv[1], &cover);
    exit_on_error(error, EXIT_ERROR, "bad cover\n");

    FILE *universe = fopen(argv[2], "r");
    exit_on_error(universe == NULL, EXIT_ERROR, "bad universe path\n");

    struct Cover supersets[2];
    uint32_t supersetsLength = sizeof(supersets)/sizeof(*supersets);
    assert(supersetsLength >= 2);

    uint32_t mask = (1UL << COVER_MAX_POLYOMINOES) - 1;
    rewind(universe);
    error = cover_find_supersets(universe, &cover, mask, &supersetsLength, supersets);
    exit_on_error(error, EXIT_ERROR, "bad universe file\n");
    assert(supersetsLength <= sizeof(supersets)/sizeof(*supersets));

    char coverString[COVER_MAX_COVER_STRING_LENGTH] = {0};

    if (supersetsLength != 1) {
        if (verbose) {
            printf("%sNOT CRITICAL%s\n", TEXT_COLOUR_FG_RED, TEXT_RESET);
            printf("cover has %u supersets\n", supersetsLength);
            for (uint32_t i = 0; i < supersetsLength; i++) {
                cover_to_string(sizeof(coverString), coverString, &supersets[i]);
                printf("%*s%s\n", 2, "", coverString);
            }
        }
        return EXIT_IS_NOT_CRITICAL;
    }

    if (verbose) {
        printf("number of supersets: %u\n", supersetsLength);
        for (uint32_t i = 0; i < supersetsLength; i++) {
            cover_to_string(sizeof(coverString), coverString, &supersets[i]);
            printf("%s\n", coverString);
        }
    }

    mask = (1UL << COVER_MAX_POLYOMINOES) - 1;
    for (uint32_t polyIndex = 0; polyIndex < cover.numPolys; polyIndex++) {
        supersetsLength = sizeof(supersets)/sizeof(*supersets);
        rewind(universe);
        error = cover_find_supersets(
                universe, &cover,
                mask & ~(1UL << polyIndex),
                &supersetsLength, supersets);
        exit_on_error(error, EXIT_ERROR, "bad universe file\n");
        assert(supersetsLength <= sizeof(supersets)/sizeof(*supersets));

        if (supersetsLength < 2) {
            if (verbose) {
                printf("%sNOT CRITICAL%s\n", TEXT_COLOUR_FG_RED, TEXT_RESET);
                printf("subset has %u supersets\n", supersetsLength);
                for (uint32_t i = 0; i < supersetsLength; i++) {
                    cover_to_string(sizeof(coverString), coverString, &supersets[i]);
                    printf("%*s%s\n", 2, "", coverString);
                }
            }
            return EXIT_IS_NOT_CRITICAL;
        }
    }

    if (verbose) {
        printf("%sCRITICAL%s\n", TEXT_COLOUR_FG_GREEN, TEXT_RESET);
    }

    return EXIT_IS_CRITICAL;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "polys.h"
#include "error.h"
#include "hash.h"


//#define POLYS_LENGTH_MAX ((1 << 30) / sizeof(uint64_t)) /* 1MB */
#define POLYS_LENGTH_MAX (1 << 17)
uint64_t polys[POLYS_LENGTH_MAX];

#define HASHES_LENGTH_MAX POLYS_LENGTH_MAX
uint64_t hashes[HASHES_LENGTH_MAX];

#define index_hash splitmix64_mix


int compare_uint64(const void *a, const void *b) {
    uint64_t left = *((uint64_t*) a);
    uint64_t right = *((uint64_t*) b);

    if (left < right) {
        return -1;
    } else if (left == right) {
        return 0;
    }
    return 1;
}

uint32_t prune(uint32_t count, uint64_t polys[count]) {
    qsort(polys, count, sizeof(*polys), compare_uint64);
    for (uint32_t i = count - 1; i >= 1; i--) {
        if (polys[i - 1] == polys[i]) {
            polys[i] = polys[count - 1];
            count--;
        }
    }
    return count;
}

int main(int argc, char *argv[argc]) {
    uint8_t error = OKAY;

    /* track number of hash collisions */
    uint32_t collisions = 0;

    /* check command line */
    exit_on_error(argc != 2, EXIT_FAILURE, "usage: hash-tester UNIVERSE\n");

    bool verbose = true;

    /* open file */
    int universe = open(argv[1], O_RDONLY);
    exit_on_error(universe < 0, EXIT_FAILURE, "Cannot open file \"%s\"\n", argv[1]);

    /* get file stats */
    struct stat universeStat;
    error = fstat(universe, &universeStat);
    exit_on_error(error, EXIT_FAILURE, "fstata failed\n");

    /* memory map file */
    uint64_t mappingLength = universeStat.st_size;
    char *mapping = mmap(NULL, mappingLength, PROT_READ, MAP_PRIVATE | MAP_POPULATE, universe, 0);
    (void) close(universe);
    exit_on_error(mapping == MAP_FAILED, EXIT_FAILURE, "mmap failed\n");

    /* read file and create polyominoes */
    if (verbose) {
        printf("reading...\n");
    }
    uint32_t polysCount = 0;
    uint32_t lineNumber = 0;
    uint64_t lineStart = 0;
    uint64_t lineEnd = 0;
    while (lineEnd < mappingLength) {
        /* find end of line */
        while (lineEnd < mappingLength && mapping[lineEnd] != '\n') {
            lineEnd++;
        }
        lineNumber++;

        /* read new polys from line */
        uint32_t polysPendingCount = 0;
        uint64_t polysPending[8] = { 0 };
        assert(lineEnd - lineStart < UINT32_MAX);
        error = polys_from_string(
                &polysPendingCount, sizeof(polysPending)/sizeof(*polysPending),
                polysPending,
                lineEnd - lineStart, &mapping[lineStart]);
        if (error) {
            if (verbose) {
                printf("Error on line %u\n", lineNumber);
            }

            error = polysPendingCount >= sizeof(polysPending)/sizeof(*polysPending);
            exit_on_error(error, EXIT_FAILURE, "polysPending too small");
        }

        /* place pending polys in poly list */
        for (uint32_t i = 0; i < polysPendingCount; i++) {
            uint64_t poly = polysPending[i];
            assert(poly != 0);
            uint32_t index = index_hash(poly) & (POLYS_LENGTH_MAX - 1);

            /* walk list to find place to put poly */
            uint32_t toSearch = POLYS_LENGTH_MAX;
            while(toSearch > 0 && polys[index] != 0 && polys[index] != poly) {
                index++;
                index %= POLYS_LENGTH_MAX;
                toSearch--;
            }

            if (polys[index] == poly) {
                continue;
            }

            exit_on_error(toSearch == 0, EXIT_FAILURE, "polys too small");

            if (toSearch != POLYS_LENGTH_MAX) {
                collisions++;
            }

            polys[index] = poly;
            polysCount++;
        }


        /* print reading status every 10k lines */
        if (false & verbose && lineNumber % 10000 == 0) {
            printf("line: %u\n", lineNumber);
        }

        /* go to start of next line */
        lineEnd++;
        lineStart = lineEnd;
    }

    printf("polyominoes: %u\n", polysCount);
    printf("collision: %u\n", collisions);
    printf("collision %%: %.2f\n", ((double) collisions / (double) polysCount) * 100.0 );

    /* clean up */
    error = munmap(mapping, mappingLength);
    exit_on_error(error, EXIT_FAILURE, "munmap failed\n");

    return EXIT_SUCCESS;
}

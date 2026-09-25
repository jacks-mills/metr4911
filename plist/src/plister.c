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


#define POLYS_LENGTH_MAX ((1 << 30) / sizeof(uint64_t)) /* 1MB */
uint64_t polys[POLYS_LENGTH_MAX];


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

int compare_by_hash(const void *a, const void *b) {
    uint64_t left = splitmix64_mix(*((uint64_t*) a)) % (1 << 17);
    uint64_t right = splitmix64_mix(*((uint64_t*) b)) % (1 << 17);

    if (left < right) {
        return -1;
    } else if (left == right) {
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[argc]) {
    uint8_t error = OKAY;

    /* check command line */
    exit_on_error(argc != 2, EXIT_FAILURE, "usage: plister UNIVERSE\n");

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

    /* read file and record polyominoes */
    if (verbose) {
        printf("reading...\n");
    }
    uint32_t polysCount = 0;
    uint32_t lineNumber = 0;
    uint64_t lineStart = 0;
    uint64_t lineEnd = 0;
    while (1) {
        while (lineEnd < mappingLength && polysCount < POLYS_LENGTH_MAX) {
            /* find end of line */
            while (lineEnd < mappingLength && mapping[lineEnd] != '\n') {
                lineEnd++;
            }
            lineNumber++;

            /* check remaining space in polys */
            uint32_t polysResidue = POLYS_LENGTH_MAX - polysCount;
            if (polysResidue < 8) {
                break;
            }

            /* read new polys from line */
            assert(lineEnd - lineStart < UINT32_MAX);
            error = polys_from_string(
                    &polysCount, POLYS_LENGTH_MAX, polys,
                    lineEnd - lineStart, &mapping[lineStart]);
            if (error && verbose) {
                printf("Error on line %u\n", lineNumber);
            }

            /* go to start of next line */
            lineEnd++;
            lineStart = lineEnd;
        }

        /* sort list of polys */
        if (verbose) {
            printf("sorting...\n");
        }
        qsort(polys, polysCount, sizeof(*polys), compare_uint64);

        /* prune duplicate polys */
        if (verbose) {
            printf("pruning...\n");
        }
        uint32_t prePruneCount = polysCount;
        for (uint32_t i = polysCount - 1; i >= 1; i--) {
            if (polys[i - 1] == polys[i]) {
                polys[i] = polys[polysCount - 1];
                polysCount--;
            }
        }
        if (prePruneCount == polysCount) {
            if (verbose) {
                printf("polys count unchanged\n");
            }
            break;
        } else if (verbose) {
            printf("polys count %u <- %u\n", polysCount, prePruneCount);
        }

        /* break if end of mapping reached */
        if (lineEnd >= mappingLength) {
            break;
        }
    }

    /* print list of polys, sorted by hash */
    qsort(polys, polysCount, sizeof(*polys), compare_by_hash);
    for (uint32_t i = 0; i < polysCount && i < POLYS_LENGTH_MAX; i++) {
        /* print polyomino */
        for (uint32_t j = 0; j < 64; j++) {
            printf("%c", polys[i] & (1UL << j) ? '1' : '0');
        }

        /* print hash */
        printf(" %lu", splitmix64_mix(polys[i]) % (1 << 17) - 1);
        printf("\n");
    }


    /* clean up */
    error = munmap(mapping, mappingLength);
    exit_on_error(error, EXIT_FAILURE, "munmap failed\n");

    return EXIT_SUCCESS;
}

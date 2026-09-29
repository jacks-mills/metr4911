#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "error.h"

#define USAGE_STR "usage: pultra [-v] UNIVERSE\n"


int main(int argc, char *argv[argc]) {
    uint8_t error = OKAY;

    /* check command line */
    exit_on_error(argc != 2 && argc != 3, EXIT_FAILURE, USAGE_STR);

    bool verbose = false;
    if (argc == 3) {
        exit_on_error(strcmp(argv[1], "-v") != 0, EXIT_FAILURE, USAGE_STR);
        verbose = true;
        argc--;
        argv++;
    }

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


    /* clean up */
    error = munmap(mapping, mappingLength);
    exit_on_error(error, EXIT_FAILURE, "munmap failed\n");

    return EXIT_SUCCESS;
}

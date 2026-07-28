#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "errors.h"
#include "cover.h"

#define PROG_NAME "pcritical"

int main(int argc, char **argv) {
    error_t error = OKAY;

    struct Cover cover = {0};
    FILE *universe = NULL;

    exit_on_error(argc != 3, EXIT_FAILURE, "Usage: %s COVER_OF_INTEREST UNIVERSE\n", PROG_NAME);

    error = cover_from_string(strlen(argv[1]), argv[1], &cover);
    exit_on_error(error, EXIT_FAILURE, "bad cover\n");
    printf("User passed cover:\n");
    cover_print(&cover);
    printf("\n");

    universe = fopen(argv[2], "r");
    exit_on_error(universe == NULL, EXIT_FAILURE, "fopen failed to open universe file\n");

    bool subsetIsCritSet[COVER_MAX_NUM_PARTIAL_COVERS] = { false };
    memset(subsetIsCritSet, true, sizeof(subsetIsCritSet));

    printf("Universe file\n");
    error = cover_status_update(universe, sizeof(subsetIsCritSet), subsetIsCritSet, &cover);
    exit_on_error(error, EXIT_FAILURE, "cover_status_update failed\n");

    printf("Subset Is Critical Set Array\n");
    printf("[");
    for (uint32_t i = 0; i < (1 << cover.order); i++) {
        printf("%c", (subsetIsCritSet[i]) ? '1' : '0');
    }
    printf("]\n");

    printf("Done\n");

    return EXIT_SUCCESS;
}

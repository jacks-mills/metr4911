#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "errors.h"
#include "cover.h"

#define PROG_NAME "pcritical"

static void status_print(uint32_t length, uint8_t status[length]) {
    printf("Critical subsets: ");
    printf("[");
    for (uint32_t i = 0; i < length; i++) {
        printf("%c", (status[i]) ? '1' : '0');
    }
    printf("]\n");
}

int main(int argc, char **argv) {
    error_t error = OKAY;

    struct Cover cover = {0};
    FILE *universe = NULL;

    exit_on_error(argc != 3, EXIT_FAILURE, "Usage: %s COVER_OF_INTEREST UNIVERSE\n", PROG_NAME);

    error = cover_from_string(strlen(argv[1]), argv[1], &cover);
    exit_on_error(error, EXIT_FAILURE, "bad cover\n");

    char coverString[COVER_MAX_COVER_STRING_LENGTH] = {0};
    cover_to_string(sizeof(coverString), coverString, &cover);

    printf("User passed cover:\n");
    printf("%s\n", coverString);
    cover_print(&cover);
    printf("\n");


    universe = fopen(argv[2], "r");
    exit_on_error(universe == NULL, EXIT_FAILURE, "fopen failed to open universe file\n");

    uint8_t subsetIsCritSet[COVER_MAX_NUM_PARTIAL_COVERS] = { false };
    memset(subsetIsCritSet, STATUS_UNDEF, sizeof(subsetIsCritSet));
    assert((1UL << cover.order) <= sizeof(subsetIsCritSet)/sizeof(*subsetIsCritSet));

    status_print((1UL << cover.order), subsetIsCritSet);

    printf("Marking intersections\n");
    error = cover_status_mark_intersections(
            (1UL << cover.order), subsetIsCritSet,
            universe, &cover);
    exit_on_error(error, EXIT_FAILURE, "cover_status_mark_intersections failed\n");
    status_print((1UL << cover.order), subsetIsCritSet);

    printf("Marking subsets\n");
    cover_status_mark_subsets((1UL << cover.order), subsetIsCritSet, &cover);
    status_print((1UL << cover.order), subsetIsCritSet);

    printf("Marking supersets\n");
    cover_status_mark_supersets((1UL << cover.order), subsetIsCritSet, &cover);
    status_print((1UL << cover.order), subsetIsCritSet);

    
    cover_print_string_from_status((1UL << cover.order), subsetIsCritSet, &cover);

    printf("Done\n");

    return EXIT_SUCCESS;
}

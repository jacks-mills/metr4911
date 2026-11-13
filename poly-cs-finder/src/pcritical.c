#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "errors.h"
#include "cover.h"

#define PROG_NAME "pcritical"


struct Context {
    FILE *universe;
    FILE *out;
    char cover[COVER_MAX_COVER_STRING_LENGTH];
    uint32_t verboseLevel;
};

void usage(void) {
    fprintf(stderr, "pcritical [-v] COVER UNIVERSE [OUT]\n");
    exit(EXIT_FAILURE);
}

error_t process_cmd_line(uint32_t argc, char *argv[argc], struct Context *context) {
    assert(argv != NULL);
    assert(context != NULL);

    context->universe = NULL;
    context->out = NULL;
    context->cover[0] = '\0';
    context->verboseLevel = 0;


    uint32_t remaining = argc;
    char **head = argv;

    if (remaining < 2) {
        return ERROR;
    }

    remaining--;
    head++;

    if (strcmp(*head, "-v") == 0) {
        context->verboseLevel = 1;
        remaining--;
        head++;
    }

    if (!(remaining == 2 || remaining == 3)) {
        return ERROR;
    }

    /* Cover argument */
    if (strlen(*head) + 1 > sizeof(context->cover)) {
        return ERROR;
    }
    strncpy(context->cover, *head, sizeof(context->cover));
    remaining--;
    head++;

    /* Universe argument */
    context->universe = fopen(*head, "r");
    if (context->universe == NULL) {
        return ERROR;
    }
    remaining--;
    head++;

    /* Out argument */
    context->out = stdout;
    if (remaining != 0) {
        context->out = fopen(*head, "w");
        if (context->out == NULL) {
            return ERROR;
        }
        remaining--;
        head++;
    }

    return OKAY;
}

static void status_print(uint32_t length, uint8_t status[length]) {
    printf("Critical subsets: ");
    printf("[");
    for (uint32_t i = 0; i < length; i++) {
        switch (status[i]) {
        case STATUS_UNDEFINED:
            printf("U");
            break;
        case STATUS_SET:
            printf("S");
            break;
        case STATUS_DEFINING_SET:
            printf("D");
            break;
        case STATUS_CRITICAL_SET:
            printf("C");
            break;
        }
    }
    printf("]\n");
}

int main(int argc, char **argv) {
    error_t error = OKAY;

    struct Context context = { 0 };
    error = process_cmd_line(argc, argv, &context);
    if (error) {
        usage();
    }

    bool verbose = (context.verboseLevel > 0);
    if (verbose) {
        printf("Context struct\n");
        printf("cover:     %s\n", context.cover);
        printf("universe:  %p\n", (void*) context.universe);
        if (context.out == NULL) {
            printf("out:       NULL\n");
        } else {
            printf("out:       %p\n", (void*) context.out);
        }
        printf("verbosity: %u\n", context.verboseLevel);
    }

    FILE *universe = context.universe;
    struct Cover cover = {0};
    error = cover_from_string(strlen(context.cover), context.cover, &cover);
    exit_on_error(error, EXIT_FAILURE, "bad cover\n");


    char coverString[COVER_MAX_COVER_STRING_LENGTH] = {0};
    cover_to_string(sizeof(coverString), coverString, &cover);

    if (verbose) {
        printf("User passed cover:\n");
        printf("%s\n", coverString);
        cover_print(&cover);
        printf("\n");
    }

    uint8_t subsetIsCritSet[COVER_MAX_NUM_PARTIAL_COVERS] = { 0 };
    memset(subsetIsCritSet, STATUS_UNDEFINED, sizeof(subsetIsCritSet));
    assert((1UL << cover.order) <= sizeof(subsetIsCritSet)/sizeof(*subsetIsCritSet));

    if (verbose) {
        status_print((1UL << cover.order), subsetIsCritSet);
    }

    /* MARK INTERSECTIONS */
    error = cover_status_mark_intersections(
            (1UL << cover.order), subsetIsCritSet,
            universe, &cover);
    exit_on_error(error, EXIT_FAILURE, "cover_status_mark_intersections failed\n");
    if (verbose) {
        printf("Marking intersections\n");
        status_print((1UL << cover.order), subsetIsCritSet);
    }

    /* MARK SUBSETS */
    cover_status_mark_subsets((1UL << cover.order), subsetIsCritSet, &cover);
    if (verbose) {
        printf("Marking subsets\n");
        status_print((1UL << cover.order), subsetIsCritSet);
    }

    /* MARK SUPERSETS */
    cover_status_mark_supersets((1UL << cover.order), subsetIsCritSet, &cover);
    if (verbose) {
        printf("Marking supersets\n");
        status_print((1UL << cover.order), subsetIsCritSet);
    }
    
    /* REPORT RESULTS */
    //fprintf(context.out, "%s\n", coverString);
    cover_print_string_from_status(context.out, (1UL << cover.order), subsetIsCritSet, &cover);
    if (verbose) {
        printf("Done\n");
    }

    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "errors.h"
#include "cover.h"

#define PROG_NAME "pcritical"


struct Context {
    FILE *covers;
    FILE *universe;
    FILE *out;
    char cover[COVER_MAX_COVER_STRING_LENGTH];
    uint32_t verboseLevel;
};

void usage(void) {
    const char *programName = "pcritical";
    const uint32_t programNameLen = 9;

    fprintf(stderr, "%*s [-v]    COVER          UNIVERSE    [OUT]\n", programNameLen, programName);
    fprintf(stderr, "%*s [-v] -c COVER       -u UNIVERSE [-o OUT]\n", programNameLen, "");
    fprintf(stderr, "%*s [-v] -C COVERS_FILE -u UNIVERSE [-o OUT]\n", programNameLen, "");
    exit(EXIT_FAILURE);
}

error_t process_cmd_line(uint32_t argc, char *argv[argc], struct Context *context) {
    assert(argv != NULL);
    assert(context != NULL);

    context->covers = NULL;
    context->universe = NULL;
    context->out = NULL;
    context->cover[0] = '\0';
    context->verboseLevel = 0;


    if (argc == 0) {
        return ERROR;
    }

    if (strcmp(argv[1], "-v") == 0) {
        context->verboseLevel = 1;
        argc--;
        argv++;
    }

    if (!(argc == 3 || argc == 4 || argc == 5 || argc == 7)) {
        return ERROR;
    }

    char *cover = NULL;
    char *coverFilePath = NULL;
    char *universeFilePath = NULL;
    char *outFilePath = NULL;

    bool isPositionalArguments = (argc == 3 || argc == 4);
    if (isPositionalArguments) {
        cover = argv[1]; 
        universeFilePath = argv[2];
        if (argc == 4) {
            outFilePath = argv[3];
        }
    } else {
        for (uint32_t i = 1; i + 1 < argc; i += 2) {
            char *option = argv[i];
            char *arg    = argv[i + 1];

            if (option[0] != '-' || option[1] == '\0' || option[2] != '\0') return ERROR;
            switch(option[1]) {
            default:
                return ERROR;
            case 'c':
                if (cover != NULL) return ERROR;
                cover = arg;
                break;
            case 'C':
                if (coverFilePath != NULL) return ERROR;
                coverFilePath = arg;
                break;
            case 'u':
                if (universeFilePath != NULL) return ERROR;
                universeFilePath = arg;
                break;
            case 'o':
                if (outFilePath != NULL) return ERROR;
                outFilePath = arg;
                break;
            }
        }
    }

    if (cover != NULL) {
        if (strlen(cover) + 1 > sizeof(context->cover)) return ERROR;
        strncpy(context->cover, cover, sizeof(context->cover));
    }

    if (coverFilePath != NULL) {
        context->covers = fopen(coverFilePath, "r");
        if (context->covers == NULL) return ERROR;
    }

    if (universeFilePath != NULL) {
        context->universe = fopen(universeFilePath, "r");
        if (context->universe == NULL) return ERROR;
    }

    context->out = stdout;
    if (outFilePath != NULL) {
        context->out = fopen(outFilePath, "w");
        if (context->out == NULL) return ERROR;
    }

    return OKAY;
}

static void status_print(uint32_t length, uint8_t status[length]) {
    printf("Critical subsets: ");
    printf("[");
    for (uint32_t i = 0; i < length; i++) {
        switch (status[i]) {
        case STATUS_UNDEFINED:
            printf("0");
            break;
        case STATUS_SET:
            printf("1");
            break;
        case STATUS_DEFINING_SET:
            printf("2");
            break;
        case STATUS_CRITICAL_SET:
            printf("3");
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
    FILE *universe = context.universe;

    if (context.covers != NULL) {
        exit_on_error(true, EXIT_FAILURE, "Not implemented covers file functionality\n");
    }

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

    error = cover_status_mark_intersections(
            (1UL << cover.order), subsetIsCritSet,
            universe, &cover);
    exit_on_error(error, EXIT_FAILURE, "cover_status_mark_intersections failed\n");
    if (verbose) {
        printf("Marking intersections\n");
        status_print((1UL << cover.order), subsetIsCritSet);
    }

    cover_status_mark_subsets((1UL << cover.order), subsetIsCritSet, &cover);
    if (verbose) {
        printf("Marking subsets\n");
        status_print((1UL << cover.order), subsetIsCritSet);
    }

    cover_status_mark_supersets((1UL << cover.order), subsetIsCritSet, &cover);
    if (verbose) {
        printf("Marking supersets\n");
        status_print((1UL << cover.order), subsetIsCritSet);
    }
    
    //printf("%s\n", coverString);
    cover_print_string_from_status(context.out, (1UL << cover.order), subsetIsCritSet, &cover);
    if (verbose) {
        printf("Done\n");
    }

    return EXIT_SUCCESS;
}

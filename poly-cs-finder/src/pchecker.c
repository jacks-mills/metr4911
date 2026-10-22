#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>


#include "errors.h"
#include "cover.h"
#include "readline.h"

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
    fprintf(stderr, "%s [-j THREADS] -f SETS_FILE UNIVERSE\n", programName);
    exit(EXIT_ERROR);
}

static int single_check(int argc, char *argv[argc]) {
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


/* ------------------------------------------------------------------------------------------------
 * Batch mode
 *
 * The universe is read into memory once, and an inverted index maps each polyomino (cell bitmask)
 * to the universe covers containing it. Checking a partial cover P then only touches covers that
 * share at least one polyomino with P. The test is the same as in single mode:
 *     P is critical  iff  exactly one universe cover contains every polyomino of P, and
 *                         for each polyomino p of P, at least two covers contain P \ {p}.
 * ----------------------------------------------------------------------------------------------*/

#define BATCH_MAX_LINE_LENGTH   256

#define RESULT_CRITICAL         0
#define RESULT_NO_COMPLETION    1
#define RESULT_MANY_COMPLETIONS 2
#define RESULT_NOT_MINIMAL      3
#define RESULT_BAD_SET          4

struct IndexEntry {
    uint64_t poly;
    uint32_t cover;
};

struct Universe {
    struct Cover *covers;
    uint32_t numCovers;
    struct IndexEntry *index;
    uint64_t indexLength;
};

struct Set {
    char string[BATCH_MAX_LINE_LENGTH];
    uint32_t lineNumber;
    uint8_t result;
    uint8_t polyIndex;      /* for RESULT_NOT_MINIMAL: the polyomino whose removal is defining */
};

struct Worker {
    pthread_t thread;
    const struct Universe *universe;
    struct Set *sets;
    uint32_t numSets;
    uint32_t first;
    uint32_t stride;
};

static int index_entry_compare(const void *a, const void *b) {
    const struct IndexEntry *x = a;
    const struct IndexEntry *y = b;
    if (x->poly != y->poly) return (x->poly < y->poly) ? -1 : 1;
    if (x->cover != y->cover) return (x->cover < y->cover) ? -1 : 1;
    return 0;
}

static error_t universe_build_index(struct Universe *universe) {
    uint64_t length = 0;
    for (uint32_t i = 0; i < universe->numCovers; i++) {
        length += universe->covers[i].numPolys;
    }

    universe->index = malloc((length ? length : 1) * sizeof(*universe->index));
    if (universe->index == NULL) return ERROR;

    uint64_t k = 0;
    for (uint32_t i = 0; i < universe->numCovers; i++) {
        for (uint32_t j = 0; j < universe->covers[i].numPolys; j++) {
            universe->index[k].poly = universe->covers[i].polys[j];
            universe->index[k].cover = i;
            k++;
        }
    }
    qsort(universe->index, length, sizeof(*universe->index), index_entry_compare);
    universe->indexLength = length;
    return OKAY;
}

/* Find the run of index entries for `poly`. Sets *begin and *end (end exclusive). */
static void universe_lookup(
        const struct Universe *universe, uint64_t poly, uint64_t *begin, uint64_t *end) {
    uint64_t lo = 0;
    uint64_t hi = universe->indexLength;
    while (lo < hi) {
        uint64_t mid = lo + (hi - lo) / 2;
        if (universe->index[mid].poly < poly) lo = mid + 1;
        else hi = mid;
    }
    *begin = lo;
    hi = universe->indexLength;
    while (lo < hi) {
        uint64_t mid = lo + (hi - lo) / 2;
        if (universe->index[mid].poly <= poly) lo = mid + 1;
        else hi = mid;
    }
    *end = lo;
}

/* containedMask[c] holds which polyominoes of the set cover c contains. It must be all zero on
 * entry and is left all zero on return. touched has room for one entry per index entry. */
static uint8_t check_set(
        const struct Universe *universe, const struct Cover *set,
        uint8_t containedMask[], uint32_t touched[], uint8_t *polyIndexOut) {
    uint32_t k = set->numPolys;
    if (k == 0) {
        /* Every cover contains the empty set. */
        if (universe->numCovers == 0) return RESULT_NO_COMPLETION;
        return (universe->numCovers == 1) ? RESULT_CRITICAL : RESULT_MANY_COMPLETIONS;
    }

    uint32_t numTouched = 0;
    for (uint32_t i = 0; i < k; i++) {
        uint64_t begin = 0;
        uint64_t end = 0;
        universe_lookup(universe, set->polys[i], &begin, &end);
        for (uint64_t e = begin; e < end; e++) {
            uint32_t c = universe->index[e].cover;
            if (containedMask[c] == 0) touched[numTouched++] = c;
            containedMask[c] |= (uint8_t) (1U << i);
        }
    }

    uint8_t full = (uint8_t) ((1U << k) - 1);
    uint32_t completions = 0;
    uint32_t withoutCount[COVER_MAX_POLYOMINOES] = { 0 };
    for (uint32_t t = 0; t < numTouched; t++) {
        uint32_t c = touched[t];
        uint8_t m = containedMask[c];
        containedMask[c] = 0;
        if (m == full) {
            completions++;
            for (uint32_t i = 0; i < k; i++) withoutCount[i]++;
        } else {
            for (uint32_t i = 0; i < k; i++) {
                if ((m | (1U << i)) == full) withoutCount[i]++;
            }
        }
    }

    if (completions == 0) return RESULT_NO_COMPLETION;
    if (completions > 1) return RESULT_MANY_COMPLETIONS;

    if (k == 1) {
        /* removing the only polyomino leaves the empty set, contained in every cover */
        withoutCount[0] = universe->numCovers;
    }
    for (uint32_t i = 0; i < k; i++) {
        if (withoutCount[i] < 2) {
            *polyIndexOut = (uint8_t) i;
            return RESULT_NOT_MINIMAL;
        }
    }
    return RESULT_CRITICAL;
}

static void *worker_run(void *arg) {
    struct Worker *worker = arg;
    const struct Universe *universe = worker->universe;

    uint8_t *containedMask = calloc(universe->numCovers ? universe->numCovers : 1, 1);
    uint32_t *touched = malloc((universe->indexLength ? universe->indexLength : 1)
                               * sizeof(*touched));
    if (containedMask == NULL || touched == NULL) {
        free(containedMask);
        free(touched);
        return worker; /* non-NULL signals failure */
    }

    for (uint32_t s = worker->first; s < worker->numSets; s += worker->stride) {
        struct Set *set = &worker->sets[s];
        struct Cover cover = {0};
        if (cover_from_string(strlen(set->string), set->string, &cover)) {
            set->result = RESULT_BAD_SET;
            continue;
        }
        set->result = check_set(universe, &cover, containedMask, touched, &set->polyIndex);
    }

    free(containedMask);
    free(touched);
    return NULL;
}

/* Letter used in `string` for polyomino `polyIndex` of the cover parsed from it. */
static char set_poly_letter(const char *string, uint32_t polyIndex) {
    struct Cover cover = {0};
    if (cover_from_string(strlen(string), string, &cover) || polyIndex >= cover.numPolys) {
        return '?';
    }
    uint64_t poly = cover.polys[polyIndex];
    uint32_t cell = 0;
    while (!(poly & (1UL << cell))) cell++;
    uint32_t position = cell + cell / cover.order;  /* skip the row separators */
    return string[position];
}

static error_t sets_read(FILE *stream, struct Set **setsOut, uint32_t *numSetsOut) {
    uint32_t capacity = 1024;
    uint32_t numSets = 0;
    struct Set *sets = malloc(capacity * sizeof(*sets));
    if (sets == NULL) return ERROR;

    char line[BATCH_MAX_LINE_LENGTH];
    uint32_t lineNumber = 0;
    for (;;) {
        uint32_t read = readline(stream, sizeof(line), line);
        if (read == 0) break;
        lineNumber++;
        if (line[read - 1] != '\n' && !feof(stream)) {
            fprintf(stderr, "line %u: too long\n", lineNumber);
            free(sets);
            return ERROR;
        }

        /* trim surrounding whitespace */
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        char *end = start + strlen(start);
        while (end > start && (end[-1] == '\n' || end[-1] == '\r'
                    || end[-1] == ' ' || end[-1] == '\t')) {
            end--;
        }
        *end = '\0';

        /* skip blank lines, comments and full covers (lines with no empty cell) */
        if (*start == '\0' || *start == '#' || strchr(start, '.') == NULL) continue;

        if (numSets == capacity) {
            capacity *= 2;
            struct Set *grown = realloc(sets, capacity * sizeof(*sets));
            if (grown == NULL) {
                free(sets);
                return ERROR;
            }
            sets = grown;
        }
        strcpy(sets[numSets].string, start);
        sets[numSets].lineNumber = lineNumber;
        sets[numSets].result = RESULT_BAD_SET;
        sets[numSets].polyIndex = 0;
        numSets++;
    }

    *setsOut = sets;
    *numSetsOut = numSets;
    return OKAY;
}

static double seconds_since(const struct timespec *start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double) (now.tv_sec - start->tv_sec) + (now.tv_nsec - start->tv_nsec) / 1e9;
}

static int batch_check(int argc, char *argv[argc]) {
    const char *setsPath = NULL;
    const char *universePath = NULL;
    long numThreads = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc && setsPath == NULL) {
            setsPath = argv[++i];
        } else if (strcmp(argv[i], "-j") == 0 && i + 1 < argc && numThreads == 0) {
            char *endp = NULL;
            numThreads = strtol(argv[++i], &endp, 10);
            if (*endp != '\0' || numThreads < 1 || numThreads > 1024) usage();
        } else if (argv[i][0] != '-' && universePath == NULL) {
            universePath = argv[i];
        } else {
            usage();
        }
    }
    if (setsPath == NULL || universePath == NULL) usage();

    if (numThreads == 0) {
        numThreads = 1;
#ifdef _SC_NPROCESSORS_ONLN
        long online = sysconf(_SC_NPROCESSORS_ONLN);
        if (online > 0) numThreads = online;
#endif
    }

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE *setsFile = fopen(setsPath, "r");
    exit_on_error(setsFile == NULL, EXIT_ERROR, "bad sets file path\n");
    struct Set *sets = NULL;
    uint32_t numSets = 0;
    error_t error = sets_read(setsFile, &sets, &numSets);
    fclose(setsFile);
    exit_on_error(error, EXIT_ERROR, "bad sets file\n");

    FILE *universeFile = fopen(universePath, "r");
    exit_on_error(universeFile == NULL, EXIT_ERROR, "bad universe path\n");
    struct Universe universe = {0};
    error = cover_universe_load(universeFile, &universe.numCovers, &universe.covers);
    fclose(universeFile);
    exit_on_error(error, EXIT_ERROR, "bad universe file\n");
    error = universe_build_index(&universe);
    exit_on_error(error, EXIT_ERROR, "out of memory\n");

    if ((uint32_t) numThreads > numSets && numSets > 0) numThreads = numSets;
    fprintf(stderr, "loaded %u sets and %u universe covers in %.2f s; checking with %ld thread%s\n",
            numSets, universe.numCovers, seconds_since(&start), numThreads,
            numThreads == 1 ? "" : "s");

    struct Worker *workers = calloc(numThreads, sizeof(*workers));
    exit_on_error(workers == NULL, EXIT_ERROR, "out of memory\n");
    for (long t = 0; t < numThreads; t++) {
        workers[t].universe = &universe;
        workers[t].sets = sets;
        workers[t].numSets = numSets;
        workers[t].first = (uint32_t) t;
        workers[t].stride = (uint32_t) numThreads;
        int rc = pthread_create(&workers[t].thread, NULL, worker_run, &workers[t]);
        exit_on_error(rc != 0, EXIT_ERROR, "could not start thread\n");
    }
    bool workerFailed = false;
    for (long t = 0; t < numThreads; t++) {
        void *ret = NULL;
        pthread_join(workers[t].thread, &ret);
        if (ret != NULL) workerFailed = true;
    }
    exit_on_error(workerFailed, EXIT_ERROR, "out of memory in worker\n");

    uint32_t numCritical = 0;
    uint32_t numNotCritical = 0;
    uint32_t numErrors = 0;
    for (uint32_t s = 0; s < numSets; s++) {
        const struct Set *set = &sets[s];
        switch (set->result) {
        case RESULT_CRITICAL:
            numCritical++;
            break;
        case RESULT_NO_COMPLETION:
            numNotCritical++;
            printf("%sNOT CRITICAL%s (line %u): \"%s\" is in no cover of the universe\n",
                   TEXT_COLOUR_FG_RED, TEXT_RESET, set->lineNumber, set->string);
            break;
        case RESULT_MANY_COMPLETIONS:
            numNotCritical++;
            printf("%sNOT CRITICAL%s (line %u): \"%s\" has more than one completion\n",
                   TEXT_COLOUR_FG_RED, TEXT_RESET, set->lineNumber, set->string);
            break;
        case RESULT_NOT_MINIMAL:
            numNotCritical++;
            printf("%sNOT CRITICAL%s (line %u): \"%s\" is not minimal "
                   "(still unique without polyomino %c)\n",
                   TEXT_COLOUR_FG_RED, TEXT_RESET, set->lineNumber, set->string,
                   set_poly_letter(set->string, set->polyIndex));
            break;
        default:
            numErrors++;
            printf("%sERROR%s (line %u): could not parse \"%s\"\n",
                   TEXT_COLOUR_FG_RED, TEXT_RESET, set->lineNumber, set->string);
            break;
        }
    }

    printf("sets checked: %u\n", numSets);
    printf("critical:     %u\n", numCritical);
    printf("not critical: %u\n", numNotCritical);
    printf("errors:       %u\n", numErrors);
    printf("elapsed:      %.2f s (%ld thread%s)\n", seconds_since(&start), numThreads,
           numThreads == 1 ? "" : "s");

    free(workers);
    free(sets);
    free(universe.covers);
    free(universe.index);

    if (numErrors > 0) return EXIT_ERROR;
    if (numNotCritical > 0) return EXIT_IS_NOT_CRITICAL;
    return EXIT_IS_CRITICAL;
}

int main(int argc, char *argv[argc]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-j") == 0) {
            return batch_check(argc, argv);
        }
    }
    return single_check(argc, argv);
}

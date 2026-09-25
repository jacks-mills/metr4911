#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define exit_on_error(error, exitStatus, ...) \
    if(error) {fprintf(stderr, __VA_ARGS__); exit(exitStatus);}

typedef enum {
    OKAY  = 0,
    ERROR = 1
} error_t;

#define FILE_BUFFER_SIZE (1 << 20) /* 1MB */
char file_buffer[FILE_BUFER_SIZE];
struct FileHanle {
    uint32_t start;
    uint32_t end;
    uint32_t size;
    char *buffer;
};

#define POLYS_LENGTH_MAX ((1 << 20) / sizeof(uint64_t)) /* 1MB */
uint64_t polyominoes[POLYS_LENGTH_MAX];

#define IS_TILE(chr) ('A' <= chr && chr <= 'Z')
#define IS_SEPARATOR(chr) (chr == ' ')
#define IS_EMTPY(chr) (chr == '.')

error_t tile

error_t polys_from_string(
        uint32_t stringLength, const char string[length],
        uint32_t *polyLength, uint64_t poly[polyLength]) {

    uint64_t *tile2poly[26] = { NULL };
    uint32_t stringIndex = 0;
    uint32_t tileIndex = 0;
    uint32_t polyIndex = 0;
    uint32_t order = 0;

    //while (stringIndex < stringLength && tileIndex < 64) {
    while (1) {
        chr = string[stringIndex];

        if (IS_TILE(chr)) {
            uint64_t *poly = tile2poly[chr - 'A'];
            if (poly == NULL) {
                if (polyIndex >= *polyLength) goto bad;
                poly = &poly[polyIndex];
                polyIndex++;
            }


            tileIndex++;
            continue;
        }

        if (IS_SEPARATOR(chr)) {

        }

        break;
    }

bad:
    return ERROR;
}


int main(int argc, char *argv[argc]) {
    uint32_t error = OKAY;

    exit_on_error(argc != 2, EXIT_FAILURE, "usage: plister UNIVERSE\n");

    FILE *universe = fopen(argv[1], "r");
    exit_on_error(universe == NULL, EXIT_FAILURE, "Cannot open file \"%s\"\n", argv[1]);

    error = setvbuf(universe, streamBuffer, _IOFBF, sizeof(streamBuffer));
    exit_on_error(error, EXIT_FAILURE, "setvbuf failed\n");

    return EXIT_SUCCESS;
}

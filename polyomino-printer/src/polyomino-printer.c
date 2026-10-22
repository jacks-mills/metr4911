#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_COLOUR_BG_BLACK            "\033[40m"
#define TEXT_COLOUR_BG_RED              "\033[41m"
#define TEXT_COLOUR_BG_GREEN            "\033[42m"
#define TEXT_COLOUR_BG_BROWN            "\033[43m"
#define TEXT_COLOUR_BG_BLUE             "\033[44m"
#define TEXT_COLOUR_BG_MAGENTA          "\033[45m"
#define TEXT_COLOUR_BG_CYAN             "\033[46m"
#define TEXT_COLOUR_BG_WHITE            "\033[47m"

#define TEXT_COLOUR_BG_BRIGHT_BLACK     "\033[100m"
#define TEXT_COLOUR_BG_BRIGHT_RED       "\033[101m"
#define TEXT_COLOUR_BG_BRIGHT_GREEN     "\033[102m"
#define TEXT_COLOUR_BG_BRIGHT_BROWN     "\033[103m"
#define TEXT_COLOUR_BG_BRIGHT_BLUE      "\033[104m"
#define TEXT_COLOUR_BG_BRIGHT_MAGENTA   "\033[105m"
#define TEXT_COLOUR_BG_BRIGHT_CYAN      "\033[106m"
#define TEXT_COLOUR_BG_BRIGHT_WHITE     "\033[107m"

#define TEXT_COLOUR_BG_RESET            "\033[49m"

#define PRINT_BUFFER_SIZE 4096
#define COMMENT_PREFIX '#'

enum State {
    DONE,
};

const char *colours[] = {
        //TEXT_COLOUR_BG_BLACK, TEXT_COLOUR_BG_RED, TEXT_COLOUR_BG_GREEN,
        TEXT_COLOUR_BG_RED, TEXT_COLOUR_BG_GREEN,
        TEXT_COLOUR_BG_BROWN, TEXT_COLOUR_BG_BLUE, TEXT_COLOUR_BG_MAGENTA,
        TEXT_COLOUR_BG_CYAN, TEXT_COLOUR_BG_WHITE,

        //TEXT_COLOUR_BG_BRIGHT_BLACK,TEXT_COLOUR_BG_BRIGHT_RED, TEXT_COLOUR_BG_BRIGHT_GREEN,
        TEXT_COLOUR_BG_BRIGHT_RED, TEXT_COLOUR_BG_BRIGHT_GREEN,
        TEXT_COLOUR_BG_BRIGHT_BROWN, TEXT_COLOUR_BG_BRIGHT_BLUE, TEXT_COLOUR_BG_BRIGHT_MAGENTA,
        TEXT_COLOUR_BG_BRIGHT_CYAN, TEXT_COLOUR_BG_BRIGHT_WHITE,

        NULL
};


int main(int argc, char **argv) {
    const char **next_colour = colours;
    const char *char2colour[26 + 26 + 1] = { NULL, [52]=TEXT_COLOUR_BG_BLACK }; /* maps a-z and A-Z to colour console codes */
    const char *colour = NULL;
    int c = 0, i = 0;

    char print_buffer[PRINT_BUFFER_SIZE] = { 0 }; /* buffer of poly cover to print */
    int print_buffer_len = 0;
    int written = 0;

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            if (print_buffer_len > 0) {
                printf("%*s\n\n", print_buffer_len, print_buffer);
            }
            goto reset;
        }

        if (print_buffer_len >= PRINT_BUFFER_SIZE) {
            printf("Polyomino too large\n\n");
            goto reset;
        }

        if (c == COMMENT_PREFIX) {
            if (print_buffer_len > 0) {
                printf("Invalid character \"%c\"\n\n", c);
                goto reset;
            }
            continue;
        }

        if (c == ' ') {
            written = snprintf(
                    print_buffer + print_buffer_len,
                    PRINT_BUFFER_SIZE - print_buffer_len,
                    "\n");

            print_buffer_len += written;
            continue;
        }

        if (!isalpha(c) && c != '.') {
            printf("Invalid character \"%c\"\n\n", c);
            goto reset;
        }

        if (next_colour == NULL) {
            printf("Ran out of colours\n\n");
            goto reset;
        }

        i = (c == '.') ? 52 : ('a' <= c && c <= 'z') ? c - 'a' : c - 'A' + 26;
        if (char2colour[i] == NULL) {
            char2colour[i] = *next_colour;
            next_colour++;
        }
        colour = char2colour[i];

        written = snprintf(
                print_buffer + print_buffer_len,
                PRINT_BUFFER_SIZE - print_buffer_len,
                "%s  %s",
                colour, TEXT_COLOUR_BG_RESET);

        print_buffer_len += written;

        continue;

reset:
        print_buffer_len = 0;
        memset(char2colour, 0, sizeof(char2colour));
        next_colour = colours;

        while (c != EOF && c != '\n') {
            c = getchar();
        }

        if (c == EOF) {
            break;
        }
    }

    printf("Done\n");
    return 0;
}

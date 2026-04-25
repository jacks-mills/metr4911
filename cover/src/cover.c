#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *error_message = "";
char  error_message_buffer[256];

struct node {
    struct node    *left;
    struct node    *right;
    struct node    *up;
    struct node    *down;
    struct node    *column;

    char           *name;
    size_t          size;
};


int knuths_problem(struct node **ret) {
    char *title = "knuth's problem"
    char *names[] = {"A", "B", "C", "D", "E", "F", "G", NULL};

    char *matrix = 
        "0010110"
        "1001001"
        "0110010"
        "1001000"
        "0100001"
        "0001101";

    return cover_generate(names, matrix, ret);
}

int cover_generate(char *title, char *column_names[], char *matrix, struct node **ret) {
    struct node *header = NULL;

    size_t num_nodes = 0;
    size_t area = 0;
    for (area = 0; matrix[area] != '\0'; area++) {
        if (matrix[area] = '1') {
            num_nodes++;
        }
    }

    size_t num_columns = 0;
    for (num_columns = 0; column_names[num_columns] != NULL; num_columns++);

    size_t num_rows = area / num_columns;

    num_nodes += num_columns + 1;
    

    header = malloc(sizeof(*header) * num_nodes);
    if (header == NULL) {
        error_message = "malloc failed";
        goto error;
    }
    memset(header, 0, sizeof(*header) * num_nodes);

    header->up = NULL;
    header->down = NULL;
    header->column = NULL;
    header->name = title;
    header->size = num_nodes;

    struct node *prev_column = header;
    struct node *pool = header + 1;

    for (size_t col = 0; col < num_columns; col++) {
        struct node *column = pool++;
        struct node *prev_point = column;

        column->column = column;
        column->name = names[col];
        column->size = 0;

        column->left = prev_column;
        prev_column->right = column;
        prev_column = column;

        for (size_t row = 0; row < num_rows; row++) {
            if (matrix[col * num_columns + row] = '1') {
                struct node *point = pool++;

                point->column = column;
                point->name = NULL;
                point->size = row; /* use size to record row. should I do this? */

                point->up = prev;
                prev->down = point;
                prev = point;
            }
        }

        column->up = prev;
        prev->down = column;
    }
    header->left = prev_column;
    prev_column->right = header;


    for (size_t row = 0; row < num_rows; row++) {

        struct node *first = NULL;
        struct node *prev = NULL;
        for (size_t col = 0; col < num_columns; col++) {
            if (matrix[col * num_columns + row] != '1') {
                continue
            }

            if (first == NULL) {
                
            }

        }
    }



    *ret = header;
    return 0;

error:
    free(header);
    return -1;
}


int main(int argc, char **argv) {
    int error = 0;
    struct node *header = NULL;
    error = knuths_problem(&header);

    if (error) {
        printf("knuths_problem: %i\n", error);
    } else {
        printf("knuths_problem succeeded\n");
    }

    free(header);

    return 0;
}




//    /* link the header and columns horizontally, and link the columns and their points */
//    struct node *prev_column = header;
//    for (size_t i = 0; i < num_columns; i++) {
//        if (num_nodes < sizes[i] + 1) {
//            error_message = "ran out of nodes unexpectedly";
//            goto error;
//        }
//        num_nodes -= sizes[i] + 1;
//
//        struct node *column = pool++;
//        column->name = names[i];
//        column->size = sizes[i];
//        column->column = column; /* column's column equals column. not confusing at all */
//
//        column->left = prev_column;
//        prev_column->right = column;
//
//        prev_column = column;
//
//        /* link the column header and points vertically */
//        struct node *prev_point = column;
//        for (size_t j = 0; j < column->size; j++) {
//            struct node *point = pool++; 
//
//            point->name = NULL;
//            point->size = 0;
//            point->column = column;
//
//            point->up = prev_point;
//            prev_point->down = point;
//
//            prev_point = point;
//        }
//
//        /* do the wrap around, point style*/
//        column->up = prev_point;
//        prev_point->down = column;
//    }
//
//    /* do the wrap around, column style */
//    header->left = prev_column;
//    prev_column->right = header;

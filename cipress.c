#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int compress(char *filename);
int extract(char *filename);

int main(int argc, char **argv)
{
    int c;

    while((c = getopt(argc, argv, "c:x:")) != -1) {
        switch(c) {
            case 'c': //compress
                compress(optarg);
                break;
            case 'x': //extract
                extract(optarg);
                break;
            default:
                //nothing to do
                break;
        }

    }

    return 0;
}

int compress(char *filename)
{
    FILE *src, *dest;
    int sparse_bit;
    char c, c_old = '\0';
    u_int32_t streak_counter = 1;
    int first_time = 0;

    src = fopen(filename, "r");
    dest = fopen(strcat(filename, ".cpr"), "wb");

    if (!src || !dest) {
        printf("Error\n");
        exit(EXIT_FAILURE);
    }

    while ((c = fgetc(src)) != -1) {

        if (!first_time) {
            first_time = 1;
            c_old = c;
            continue;
        }
        if (c == c_old) {
            streak_counter++;
        } else {
            if ( streak_counter > 3) {
                /*must repeat at least 3 times*/
                fprintf(dest, "|%u%c", streak_counter,c_old);
            } else {
                /*else, dont encode but just store them in compressed file*/
                for (size_t i = 0; i < streak_counter; i++) {
                    fprintf(dest, "%c", c_old);
                }
            }
            c_old = c;
            streak_counter = 1;
        }
    }
    if ( streak_counter > 3) {
        /*must repeat at least 3 times*/
        fprintf(dest, "|%u%c", streak_counter,c_old);
    } else {
        /*else, dont encode but just store them in compressed file*/
        for (size_t i = 0; i < streak_counter; i++) {
            fprintf(dest, "%c", c_old);
        }
    }
    fclose(src);
    fclose(dest);

    return 0;
}

int extract(char *filename)
{
    FILE *src, *dest;
    int sparse_bit;
    char c, c_old = 0;
    u_int32_t streak_counter = 0;

    src = fopen(filename, "r");
    filename[strlen(filename) - 4] = '\0';
    dest = fopen(filename, "wb");

    if (!src || !dest) {
        printf("Error\n");
        exit(EXIT_FAILURE);
    }

    while ((c = fgetc(src)) != -1) {
        if (c == '|') {
            fscanf(src, "%u%c", &streak_counter, &c);
            for (size_t i = 0; i < streak_counter; i++) {
                fputc(c, dest);
            }
        } else {
            fputc(c, dest);
        }
    }
    fclose(src);
    fclose(dest);

    return 0;
}
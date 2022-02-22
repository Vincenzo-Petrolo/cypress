#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int compress(char *filename);
int extract(char *filename);
int compress_longest_seq(char *filename);

int main(int argc, char **argv)
{
    int c;

    while ((c = getopt(argc, argv, "c:x:")) != -1)
    {
        switch (c)
        {
        case 'c': // compress
            compress_longest_seq(optarg);
            break;
        case 'x': // extract
            // extract(optarg);
            break;
        default:
            // nothing to do
            break;
        }
    }

    return 0;
}

/*This compression method searches the sequence on n bits which
appears the most times and replaces it with a shorter sequence on m bits (m < n)
which appears the less times*/
#define N 2 // bytes
#define M 1 // byte
#define Ls (1 << 8 * N)
#define Ss (1 << 8 * M)
int compress_longest_seq(char *filename)
{
    FILE *src, *dest;
    int sparse_bit;
    char c, c_old = '\0';
    u_int32_t streak_counter = 1;
    int first_time = 0;
    u_int64_t longest_sequences[Ls] = {0}; // this might become cumbersome
    u_int64_t shortest_sequence[Ss] = {0}; // this too
    u_int64_t max = 0, min = __UINT64_MAX__;
    u_int16_t long_buffer;
    u_int8_t short_buffer;

    src = fopen(filename, "rb");
    dest = fopen(strcat(filename, ".cpr"), "wb");

    if (!src || !dest)
    {
        perror("Error\n");
        exit(EXIT_FAILURE);
    }

    while (fread((u_int8_t *)(&long_buffer), sizeof(u_int8_t), 2, src) == 2)
    {
        longest_sequences[long_buffer]++;
    }

    rewind(src);

    while (fread(&short_buffer, sizeof(u_int8_t), 1, src) == 1)
    {
        shortest_sequence[short_buffer]++;
    }

    /*Search max and minimum*/
    for (size_t i = 0; i < Ls; i++)
    {
        if (longest_sequences[i] > max)
        {
            max = longest_sequences[i];
            long_buffer = i;
        }
    }

    for (size_t i = 0; i < Ss; i++)
    {
        if (shortest_sequence[i] < min)
        {
            min = longest_sequences[i];
            short_buffer = i;
        }
    }

    /*Write info for reconstruction*/
    fwrite((u_int8_t *)(&long_buffer), sizeof(u_int8_t), 2, dest);
    fwrite(&short_buffer, sizeof(u_int8_t), 1, dest);

    rewind(src);
    /*Read again and replace*/
    u_int16_t buff;
    while (fread((u_int8_t *)(&buff), sizeof(u_int8_t), 2, src) == 2)
    {
        if (buff == long_buffer)
        {
            fwrite(&short_buffer, sizeof(u_int8_t), 1, dest);
        }
        else
        {
            /*Search the short sequence patter in the 2 bytes read*/
            if (short_buffer == ((u_int8_t *)(&buff))[0])
            {
                fwrite(&long_buffer, sizeof(u_int8_t), 2, dest);
                fwrite(&(((u_int8_t *)(&buff))[1]), sizeof(u_int8_t), 1, dest);
            }
            else if (short_buffer == ((u_int8_t *)(&buff))[1])
            {
                fwrite(&(((u_int8_t *)(&buff))[0]), sizeof(u_int8_t), 1, dest);
                fwrite(&long_buffer, sizeof(u_int8_t), 2, dest);
            }
            else
            {
                /*if the short sequence is not found, then simply copy from src to dest*/
                fwrite((u_int8_t *)&buff, sizeof(u_int8_t), 2, dest);
            }
        }
    }

    printf("Saved around %lu bytes with compression\n", (max - min));

    fclose(src);
    fclose(dest);

    return 0;
}

int compress_rle(char *filename)
{
    FILE *src, *dest;
    int sparse_bit;
    char c, c_old = '\0';
    u_int32_t streak_counter = 1;
    int first_time = 0;

    src = fopen(filename, "r");
    dest = fopen(strcat(filename, ".cpr"), "wb");

    if (!src || !dest)
    {
        printf("Error\n");
        exit(EXIT_FAILURE);
    }

    while ((c = fgetc(src)) != -1)
    {

        if (!first_time)
        {
            first_time = 1;
            c_old = c;
            continue;
        }
        if (c == c_old)
        {
            streak_counter++;
        }
        else
        {
            if (streak_counter > 3)
            {
                /*must repeat at least 3 times*/
                fprintf(dest, "|%u%c", streak_counter, c_old);
            }
            else
            {
                /*else, dont encode but just store them in compressed file*/
                for (size_t i = 0; i < streak_counter; i++)
                {
                    fprintf(dest, "%c", c_old);
                }
            }
            c_old = c;
            streak_counter = 1;
        }
    }
    if (streak_counter > 3)
    {
        /*must repeat at least 3 times*/
        fprintf(dest, "|%u%c", streak_counter, c_old);
    }
    else
    {
        /*else, dont encode but just store them in compressed file*/
        for (size_t i = 0; i < streak_counter; i++)
        {
            fprintf(dest, "%c", c_old);
        }
    }
    fclose(src);
    fclose(dest);

    return 0;
}

int extract_rle(char *filename)
{
    FILE *src, *dest;
    int sparse_bit;
    char c, c_old = 0;
    u_int32_t streak_counter = 0;

    src = fopen(filename, "r");
    filename[strlen(filename) - 4] = '\0';
    dest = fopen(filename, "wb");

    if (!src || !dest)
    {
        printf("Error\n");
        exit(EXIT_FAILURE);
    }

    while ((c = fgetc(src)) != -1)
    {
        if (c == '|')
        {
            fscanf(src, "%u%c", &streak_counter, &c);
            for (size_t i = 0; i < streak_counter; i++)
            {
                fputc(c, dest);
            }
        }
        else
        {
            fputc(c, dest);
        }
    }
    fclose(src);
    fclose(dest);

    return 0;
}
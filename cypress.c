#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define OVERHEAD 3 // bytes for each compression
#define N 2        // bytes
#define M 1        // byte
#define Ls (1 << 8 * N)
#define Ss (1 << 8 * M)
#define unpredictable_random_name "__c_y_press_020141__"
/**
 * TODO compress multiple times
 * until the delta of saved bytes
 * reaches 0.
 */

int compress_longest_seq(char *filename);
int extract_longest_seq(char *filename);
int commit_file(char *filename, int times);
int get_compression_times(char *filename);

int main(int argc, char **argv)
{
    int c;
    u_int64_t bytes_saved = 0;
    u_int64_t total_bytes_saved = 0;
    u_int64_t accumulated_overhead = 0;
    int compression_times = 0;

    while ((c = getopt(argc, argv, "c:x:")) != -1)
    {
        switch (c)
        {
        case 'c': // compress
            do
            {
                /*Apply compression many times until i reach the maximum*/
                bytes_saved = compress_longest_seq(optarg);
                total_bytes_saved += bytes_saved;
                accumulated_overhead += OVERHEAD;
                compression_times++;
            } while (compression_times != 2);
            printf("Saved %lu bytes\n", (total_bytes_saved - accumulated_overhead + 4));
            commit_file(optarg, compression_times);
            exit(EXIT_SUCCESS);
            break;
        case 'x': // extract
            compression_times = get_compression_times(optarg);
            while (compression_times > 0)
            {
                compression_times--;
                extract_longest_seq(optarg);
            }
            exit(EXIT_SUCCESS);
            break;
        default:
            // nothing to do
            break;
        }
    }

    return 0;
}

int commit_file(char *filename, int times)
{
    FILE *src, *dest;
    char buff;

    rename(filename, unpredictable_random_name);
    src = fopen(unpredictable_random_name, "rb");
    dest = fopen(filename, "wb");
    /*Write number of times compression has occurred*/
    fwrite(&times, sizeof(int), 1, dest);
    /*Copy everything to a new file*/
    while ((buff = fgetc(src)) != EOF)
    {
        fputc(buff, dest);
    }

    fclose(src);
    fclose(dest);
    remove(unpredictable_random_name);

    return 0;
}

/*This compression method searches the sequence on n bits which
appears the most times and replaces it with a shorter sequence on m bits (m < n)
which appears the less times*/
int compress_longest_seq(char *filename)
{
    FILE *src, *dest;
    u_int64_t longest_sequences[Ls] = {0}; // this might become cumbersome
    u_int64_t shortest_sequence[Ss] = {0}; // this too
    u_int64_t max = 0, min = __UINT64_MAX__;
    u_int8_t long_buffer[N];
    u_int8_t buff[M];
    u_int8_t longer_buff[N];
    u_int8_t short_buffer[M];
    const char *last_four = &filename[strlen(filename) - 4];

    /*If .cpr is already present at the end, then don't add*/
    if (strcmp(last_four, ".cpr") == 0)
    {
        rename(filename, unpredictable_random_name);
        src = fopen(unpredictable_random_name, "rb");
        dest = fopen(filename, "wb");
    }
    else
    {
        src = fopen(filename, "rb");
        dest = fopen(strcat(filename, ".cpr"), "wb");
    }

    if (!src || !dest)
    {
        perror("Error\n");
        exit(EXIT_FAILURE);
    }

    /*Fill the buffer with first 2 bytes*/
    fread(&longer_buff, sizeof(u_int8_t), 2, src);

    while (fread(buff, sizeof(u_int8_t), 1, src) == 1)
    {
        int tmp = ((((u_int16_t)longer_buff[0]) << 8) | (u_int16_t)longer_buff[1]);
        longest_sequences[tmp]++;

        /*update the new buffer with two most recent bytes*/
        longer_buff[0] = longer_buff[1];
        longer_buff[1] = buff[0];
    }

    rewind(src);

    while (fread(short_buffer, sizeof(u_int8_t), 1, src) == 1)
    {
        shortest_sequence[short_buffer[0]]++;
    }

    /*Search max and minimum*/
    for (size_t i = 0; i < Ls; i++)
    {
        if (longest_sequences[i] > max)
        {
            max = longest_sequences[i];
            long_buffer[1] = ((u_int16_t)i) & 0x00FF;
            long_buffer[0] = (((u_int16_t)i) >> 8) & 0x00FF;
        }
    }

    for (size_t i = 0; i < Ss; i++)
    {
        if (shortest_sequence[i] < min)
        {
            min = shortest_sequence[i];
            short_buffer[0] = (u_int8_t)i;
        }
    }

    /*Write info for reconstruction*/
    fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
    fwrite(short_buffer, sizeof(u_int8_t), 1, dest);

    rewind(src);
    /*Read again and replace*/
    /*Fill the buffer with first 2 bytes*/
    fread(longer_buff, sizeof(u_int8_t), 2, src);

    while (fread(buff, sizeof(u_int8_t), 1, src) == 1)
    {
        if (longer_buff[0] == long_buffer[0] && longer_buff[1] == long_buffer[1])
        {
            fwrite(short_buffer, sizeof(u_int8_t), 1, dest);
            /*Refill the buffer*/
            longer_buff[0] = longer_buff[1];
            longer_buff[1] = buff[0];
            fread(buff, sizeof(u_int8_t), 1, src);
            longer_buff[0] = longer_buff[1];
            longer_buff[1] = buff[0];
            continue;
        }
        else
        {
            if (short_buffer[0] == long_buffer[0])
            {
                fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
            }
            else
            {
                fwrite(&(longer_buff[0]), sizeof(u_int8_t), 1, dest);
            }
        }
        /*update the new buffer with two most recent bytes*/
        longer_buff[0] = longer_buff[1];
        longer_buff[1] = buff[0];
    }
    if (short_buffer[0] == longer_buff[0])
    {
        fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
    }
    else
    {
        fwrite(&(longer_buff[0]), sizeof(u_int8_t), 1, dest);
    }
    longer_buff[0] = longer_buff[1];
    longer_buff[1] = buff[0];

    if (short_buffer[0] == longer_buff[0])
    {
        fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
    }
    else
    {
        fwrite(&(longer_buff[0]), sizeof(u_int8_t), 1, dest);
    }

    fclose(src);
    fclose(dest);
    remove(unpredictable_random_name);
    /*Return how many bytes i saved*/
    return (max - min);
}

int get_compression_times(char *filename)
{
    FILE *src, *dest;
    int compression_times;
    char c;

    rename(filename, unpredictable_random_name);
    src = fopen(unpredictable_random_name, "r");
    dest = fopen(filename, "w");

    if (!src || !dest)
    {
        perror("Error:");
        exit(EXIT_FAILURE);
    }
    /*Strip the integer at the beginning*/
    fread(&compression_times, sizeof(int), 1, src);
    /*Copy the file in a new file*/
    while ((c = fgetc(src)) != EOF)
    {
        fputc(c, dest);
    }

    fclose(src);
    fclose(dest);
    remove(unpredictable_random_name);

    return compression_times;
}

int extract_longest_seq(char *filename)
{

    FILE *src, *dest;
    u_int8_t long_buffer[N];
    u_int8_t buff[M];
    u_int8_t longer_buff[N];
    u_int8_t short_buffer[M];
    const char *last_four = &filename[strlen(filename) - 4];

    if (strcmp(last_four, ".cpr") == 0)
    {
        /*It means i'm decompressing a .cpr file, so first time decompressing it*/
        src = fopen(filename, "r");
        filename[strlen(filename) - 4] = '\0';
    }
    else
    {
        rename(filename, unpredictable_random_name);
        src = fopen(unpredictable_random_name, "r");
    }

    dest = fopen(filename, "wb");

    if (!src || !dest)
    {
        perror("Error on opening the files");
        exit(EXIT_FAILURE);
    }

    /*Read metadata used for compression*/
    fread(long_buffer, sizeof(u_int8_t), 2, src);
    fread(short_buffer, sizeof(u_int8_t), 1, src);

    /*Fill the buffer with first 2 bytes*/
    fread(longer_buff, sizeof(u_int8_t), 2, src);

    while (fread(buff, sizeof(u_int8_t), 1, src) == 1)
    {
        if (longer_buff[0] == long_buffer[0] && longer_buff[1] == long_buffer[1])
        {
            fwrite(short_buffer, sizeof(u_int8_t), 1, dest);
            longer_buff[0] = longer_buff[1];
            longer_buff[1] = buff[0];
            fread(buff, sizeof(u_int8_t), 1, src);
            longer_buff[0] = longer_buff[1];
            longer_buff[1] = buff[0];
            continue;
        }
        else
        {
            if (short_buffer[0] == longer_buff[0])
            {
                fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
            }
            else
            {
                fwrite(&(longer_buff[0]), sizeof(u_int8_t), 1, dest);
            }
        }
        /*update the new buffer with two most recent bytes*/
        longer_buff[0] = longer_buff[1];
        longer_buff[1] = buff[0];
    }
    if (short_buffer[0] == long_buffer[0])
    {
        fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
    }
    else
    {
        fwrite(&(longer_buff[0]), sizeof(u_int8_t), 1, dest);
    }
    longer_buff[0] = longer_buff[1];
    longer_buff[1] = buff[0];

    if (short_buffer[0] == longer_buff[0])
    {
        fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
    }
    else
    {
        fwrite(&(longer_buff[0]), sizeof(u_int8_t), 1, dest);
    }

    fclose(src);
    fclose(dest);
    remove(unpredictable_random_name);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/**
 * TODO compress multiple times
 * until the delta of saved bytes
 * reaches 0.
 */

int compress_longest_seq(char *filename);
int extract_longest_seq(char *filename);

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
            extract_longest_seq(optarg);
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
    u_int64_t longest_sequences[Ls] = {0}; // this might become cumbersome
    u_int64_t shortest_sequence[Ss] = {0}; // this too
    u_int64_t max = 0, min = __UINT64_MAX__;
    u_int8_t long_buffer[N];
    u_int8_t buff[M];
    u_int8_t longer_buff[N];
    u_int8_t short_buffer[M];

    src = fopen(filename, "rb");
    dest = fopen(strcat(filename, ".cpr"), "wb");

    if (!src || !dest)
    {
        perror("Error\n");
        exit(EXIT_FAILURE);
    }

    /*Fill the buffer with first 2 bytes*/
    fread(&longer_buff, sizeof(u_int8_t), 2, src);

    while (fread(buff, sizeof(u_int8_t), 1, src) == 1)
    {
        int tmp = ((((u_int16_t) longer_buff[0]) << 8) | (u_int16_t)longer_buff[1]);
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
            min = longest_sequences[i];
            short_buffer[0] = i;
        }
    }

    /*Write info for reconstruction*/
    fwrite(long_buffer, sizeof(u_int8_t), 2, dest);
    fwrite(short_buffer, sizeof(u_int8_t), 1, dest);

    rewind(src);
    /*Read again and replace*/
    /*Fill the buffer with first 2 bytes*/
    fread(longer_buff, sizeof(u_int8_t), 2, src);

    while (fread(buff, sizeof(u_int8_t), 1, src) == 1) {
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
    printf("Saved around %lu bytes with compression\n", (max - min));

    fclose(src);
    fclose(dest);

    return 0;
}
int extract_longest_seq(char *filename)
{

    FILE *src, *dest;
    u_int8_t long_buffer[N];
    u_int8_t buff[M];
    u_int8_t longer_buff[N];
    u_int8_t short_buffer[M];


    src = fopen(filename, "r");
    filename[strlen(filename) - 4] = '\0';
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

    while (fread(buff, sizeof(u_int8_t), 1, src) == 1) {
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


    return 0;
}
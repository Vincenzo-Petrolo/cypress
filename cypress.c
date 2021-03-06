#include "cypress.h"
#include <string.h>
#include <stdio.h>

static void _print_hash(cypress_t *cpr)
{
    for (size_t i = 0; i < POSSIBLE_N_SEQ; i++)
    {
        if (cpr->file_statistics.N_seq_cnt[i] != 0) {
            printf("[%ld] -> %d\n", i, cpr->file_statistics.N_seq_cnt[i]);
        }
    }
    
}
/*Build the map for the sequences*/
static void _hash_build(cypress_t *cpr, char *filename)
{
    int first = 1;
    char buf;
    FILE *fp = fopen(filename, "rb");
    char lbuf[2], sbuf[1];

    while (fread(&buf, sizeof(char), 1, fp) == 1)
    {
        if (first)
        {
            lbuf[0] = buf;
            first = 0;
        }
        else
        {
            lbuf[1] = lbuf[0];
            lbuf[0] = buf;
            int i = (lbuf[1] << 8) + lbuf[0];
            cpr->file_statistics.N_seq_cnt[i]++;
        }
    }

    fclose(fp);
    fp = fopen(filename, "rb");

    while (fread(&buf, sizeof(char), 1, fp) == 1)
    {
        sbuf[0] = buf;
        int i = sbuf[0];
        cpr->file_statistics.M_seq_cnt[i]++;
    }

    fclose(fp);

    // Stats are built, return
    return;
}

static void _find_max_min(cypress_t *cpr)
{
    unsigned int max = 0, min = __UINT32_MAX__;

    for (size_t i = 0; i < POSSIBLE_N_SEQ; i++)
    {
        if (cpr->file_statistics.N_seq_cnt[i] > max)
        {
            cpr->file_statistics.max[0] = (char)(i & 0xFF);   // Pick rightmost 8 bits
            cpr->file_statistics.max[1] = (char)((i & 0xFF00) >> 8); // Pick leftmost 8 bits
            max = cpr->file_statistics.N_seq_cnt[i];
        }

        if (i < 256)
        {
            if (cpr->file_statistics.M_seq_cnt[i] < min)
            {
                cpr->file_statistics.min[0] = (char)i;
                min = cpr->file_statistics.M_seq_cnt[i];
            }
        }
    }

    return;
}

static void _write_overhead(cypress_t *cpr, FILE *fp)
{
    /*Write the first two bytes, and the 1 byte*/
    fwrite(&(cpr->file_statistics.max[1]), sizeof(char), 1, fp);
    fwrite(&(cpr->file_statistics.max[0]), sizeof(char), 1, fp);
    fwrite(&(cpr->file_statistics.min[0]), sizeof(char), 1, fp);

    // For each compression i add an overhead equal to the bytes needed to compress it.
    cpr->accumulated_overhead += 3;

}

static void _replace(cypress_t *cpr, char *filename)
{
    /*Create a new version of the file with .cpr as extension*/
    FILE *cpr_fp, *fp;
    int first = 1;
    char buf;
    char lbuf[2];

    /*Open the files*/
    fp = fopen(filename, "rb");
    cpr_fp = fopen(strcat(filename, ".cpr"), "w");

    /*Before everything write the infos to allow decompression in the file*/
    _write_overhead(cpr, cpr_fp);
    /*Read from source file*/
    while (fread(&buf, sizeof(char), 1, fp) == 1)
    {
        if (first)
        {
            // Just fill the buffer at the beginning
            lbuf[0] = buf;
            first = 0;
        }
        else
        {
            lbuf[1] = lbuf[0];
            lbuf[0] = buf;
            if (lbuf[1] == cpr->file_statistics.max[1] && lbuf[0] == cpr->file_statistics.max[0])
            {
                // I found a sequence that matches
                // Transfer the shorter byte into the file
                fwrite(&(cpr->file_statistics.min[0]), sizeof(char), 1, cpr_fp);
                // Need to refill the buffer
                first = 1;
                // I hit a sequence, therefore i save 1 byte
                cpr->saved_bytes++;
            }
            else
            {
                // The current sequence does not match
                // Transfer the oldest byte to the destination file
                fwrite(&lbuf[1], sizeof(char), 1, cpr_fp);
            }
        }
    }
    /*After i need to write the last byte i read*/
    fwrite(&lbuf[0], sizeof(char), 1, cpr_fp);

    fclose(cpr_fp);
    fclose(fp);

    return;
}

/*Perform compression of the given file, and return the saved bytes*/
int compress(cypress_t *cpr, char *filename)
{
    /*First build the hash map for the sequences in the file*/
    printf("Performing hash build\n");
    _hash_build(cpr, filename);
    /*Now find 2 byte maximum frequency seq, and 1 byte minimum frequency seq*/
    printf("Finding max and min\n");
    _find_max_min(cpr);
    /*Now perform the replacement*/
    printf("Performing replacement\n");
    _replace(cpr, filename);

    /*Increase the compression times*/
    cpr->compression_times += 1;

    return cpr->saved_bytes;
}

static void _read_overhead(cypress_t *cpr, char *filename)
{
    FILE *cpr_fp = fopen(filename, "rb");
    char buf;
    int res;
    res = fread(&buf, sizeof(char), 1, cpr_fp);
    cpr->file_statistics.max[1] = buf;
    res = fread(&buf, sizeof(char), 1, cpr_fp);
    cpr->file_statistics.max[0] = buf;
    res = fread(&buf, sizeof(char), 1, cpr_fp);
    cpr->file_statistics.min[0] = buf;

    fclose(cpr_fp);

    return;
}

static void _reconstruct(cypress_t *cpr, char *filename)
{
    FILE *cpr_fp, *fp;
    cpr_fp = fopen(filename, "rb");
    char *new_filename = strtok(filename, ".");
    fp = fopen(new_filename, "w");
    char buf;

    //Remove the first 3 bytes by performing dummy reads
    buf = fread(&buf, sizeof(char), 1, cpr_fp);
    buf = fread(&buf, sizeof(char), 1, cpr_fp);
    buf = fread(&buf, sizeof(char), 1, cpr_fp);

    while (fread(&buf, sizeof(char), 1, cpr_fp) == 1)
    {
        if (buf == cpr->file_statistics.min[0]) {
            /*Write instead the full length of bytes*/
            fwrite(&(cpr->file_statistics.max[1]), sizeof(char),1, fp);
            fwrite(&(cpr->file_statistics.max[0]), sizeof(char),1, fp);
        } else {
            fwrite(&buf, sizeof(char), 1, fp);
        }
    }
    

    fclose(cpr_fp);
    fclose(fp);

    return;
}

void extract(cypress_t *cpr, char *filename)
{
    /*Read the first 3 bytes*/
    _read_overhead(cpr, filename);
    /*Perform the reconstruction of the original file*/
   _reconstruct(cpr, filename);

   return;
}

void init(cypress_t *cpr)
{
    cpr->accumulated_overhead = 0;
    cpr->compression_times = 0;
    cpr->saved_bytes = 0;

    for (size_t i = 0; i < POSSIBLE_N_SEQ; i++)
    {
        cpr->file_statistics.N_seq_cnt[i] = 0;
        if (i < 256) {
            cpr->file_statistics.M_seq_cnt[i] = 0;
        }
    }
    

    return;
}
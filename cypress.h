#ifndef __CYPRESS__
#define __CYPRESS__

#define POSSIBLE_N_SEQ 256*256
#define POSSIBLE_M_SEQ 256

#define OVERHEAD 10

typedef struct 
{
    unsigned int N_seq_cnt[POSSIBLE_N_SEQ];
    unsigned int M_seq_cnt[POSSIBLE_M_SEQ];
    char max[2];
    char min[1];
} stats_t;

typedef struct
{
    int saved_bytes;
    int compression_times;
    int accumulated_overhead;
    stats_t file_statistics;
} cypress_t;

int compress(cypress_t *cpr, char *filename);
void init(cypress_t *cpr);


#endif
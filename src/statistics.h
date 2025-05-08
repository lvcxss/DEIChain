#ifndef STATISTICS_H
#define STATISTICS_H

void print_statistics();
typedef struct {
    long int miners_id;
    long int invalid_blocks;
    long int valid_blocks;
    long int miners_credit;
  }MinerInfo;
#endif // !DEBUG

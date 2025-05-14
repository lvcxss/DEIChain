#ifndef STATISTICS_H
#define STATISTICS_H

#include "deichain.h"
#include <signal.h>
#include <sys/msg.h>
#include <time.h>

typedef struct {
  int valid_blocks;
  int invalid_blocks;
  int total_credits;
  double total_processing_time;
  int total_transactions;
} MinerStats;

extern volatile sig_atomic_t usr1_received;
extern MinerStats *miners_stats;
extern int total_valid_blocks;
extern int total_invalid_blocks;

void print_current_stats();
void handle_sigusr1(int sig);
void print_statistics();

#endif

// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include <pthread.h>
#include <stdio.h>

void showBlock(Block *block, Transaction *t) {
  printf("Block : %s\n", block->block_id);
  printf("Previous Hash : %s\n", block->previous_hash);
  printf("Nonce : %d\n", block->nonce);
  printf("Timestamp: %ld\n", block->timestamp);
  printf("Transactions :\n");
  for (unsigned int i = 0; i < config.transactions_per_block; i++) {
    printf("       [%d] ID: %s | Reward: %d | Value: %f | Timestamp: %ld\n", i,
           t[i].transaction_id, t[i].reward, t[i].value, t[i].timestamp);
  }
}

int write_logfile(char *message, char *typemsg) {
  time_t raw = time(NULL);
  struct tm timeinfo;
  if (raw == -1) {
    printf("Error: Could not get current time\n");
    return -1;
  }
  if (localtime_r(&raw, &timeinfo) == NULL) {
    return -2;
  }
  char timestamp[20];
  if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo) ==
      0) {
    return -3;
  }
  // lock only here to reduce concorrency problems
  if (log_file_mutex != NULL)
    sem_wait(log_file_mutex);

  FILE *logfile = fopen("DEIChain_log.log", "a");
  if (logfile == NULL) {
    printf("Error: Could not open file logfile.txt\n");
    if (log_file_mutex != NULL)
      sem_post(log_file_mutex);

    return -4;
  }
  fprintf(logfile, "%s %s: %s\n", timestamp, typemsg, message);
  fclose(logfile);
  if (log_file_mutex != NULL)
    sem_post(log_file_mutex);

  return 0;
}

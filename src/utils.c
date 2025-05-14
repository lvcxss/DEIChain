// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include <pthread.h>
#include <stdio.h>

void show_block(Block *block, Transaction *t) {
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

int write_ledger_logfile() {
  if (log_file_mutex != NULL)
    sem_wait(log_file_mutex);

  FILE *logfile = fopen("DEIChain_log.log", "a");
  if (logfile == NULL) {
    printf("Error: Could not open file logfile.txt\n");
    if (log_file_mutex != NULL)
      sem_post(log_file_mutex);

    return -4;
  }

  Block *blocks = (Block *)((char *)block_ledger + sizeof(BlockchainLedger));
  for (int i = 0; i < block_ledger->num_blocks; i++) {
    Block *b = (Block *)((char *)blocks +
                         (sizeof(Transaction) * config.transactions_per_block +
                          sizeof(Block)) *
                             i);
    Transaction *blcktrans = (Transaction *)((char *)b + sizeof(Block));
    fprintf(logfile, "Block : %s\n", b->block_id);
    fprintf(logfile, "Previous Hash : %s\n", b->previous_hash);
    fprintf(logfile, "Nonce : %d\n", b->nonce);
    fprintf(logfile, "Timestamp: %ld\n", b->timestamp);
    fprintf(logfile, "Transactions :\n");
    for (unsigned int ii = 0; ii < config.transactions_per_block; ii++) {
      fprintf(logfile,
              "       [%d] ID: %s | Reward: %d | Value: %f | Timestamp: %ld\n",
              i, blcktrans[ii].transaction_id, blcktrans[ii].reward,
              blcktrans[ii].value, blcktrans[ii].timestamp);
    }
    if (log_file_mutex != NULL)
      sem_post(log_file_mutex);
  }
  fclose(logfile);
  return 0;
}

// by now this file just has this function but it will have more general
// functions
int write_logfile(char *message, char *typemsg) {

  // timestamp for the logfile
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

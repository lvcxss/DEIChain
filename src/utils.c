// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void showBlock(Block *block) {
  printf("Block : %s\n", block->block_id);
  printf("Previous Block : %s\n", block->previous_hash);
  printf("Nonce : %d\n", block->nonce);
  printf("Timestamp: %ld\n", block->timestamp);
}
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
  // lock only here to reduce concorrency problems
  if (log_file_mutex != NULL)
    sem_wait(log_file_mutex);

  FILE *logfile = fopen("DEIChain_log.txt", "a");
  if (logfile == NULL) {
    printf("Error: Could not open file logfile.txt\n");
    if (log_file_mutex != NULL)
      sem_post(log_file_mutex);

    return -4;
  }
  fprintf(logfile, "[%s] (%s) %s\n", typemsg, timestamp, message);
  fclose(logfile);
  if (log_file_mutex != NULL)
    sem_post(log_file_mutex);

  return 0;
}

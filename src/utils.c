// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t logfilemutex = PTHREAD_MUTEX_INITIALIZER;

void showBlock(Block *block) {
  printf("Block : %s\n", block->block_id);
  printf("Previous Block : %s\n", block->previous_hash);
  printf("Nonce : %d\n", block->nonce);
  printf("Timestamp: %ld\n", block->timestamp);
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
  // lock only here to reduce concorrency problems
  pthread_mutex_lock(&logfilemutex);
  FILE *logfile = fopen("DEIChain_log.txt", "a");
  if (logfile == NULL) {
    printf("Error: Could not open file logfile.txt\n");
    pthread_mutex_unlock(&logfilemutex);
    return -4;
  }
  fprintf(logfile, "[%s] (%s) %s\n", typemsg, timestamp, message);
  fclose(logfile);
  pthread_mutex_unlock(&logfilemutex);
  return 0;
}

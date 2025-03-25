#include "miner.h"
#include "transaction.h"
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <wait.h>
#define BUFFER_SIZE 256

void init();
void terminate(int shmid);
void processFile(int **arr, char *filename);
sem_t *logfilesem;

void init() {
  char *filename = "config.cfg";
  int num_miners, pool_size, blockchain_blocks, transaction_pool_size;
  int *ar[] = {&num_miners, &pool_size, &blockchain_blocks,
               &transaction_pool_size};
  // read file info
  processFile(ar, filename);
  printf("%d %d %d %d\n", num_miners, pool_size, blockchain_blocks,
         transaction_pool_size);

  // semaphores
  sem_unlink("LOGFILE");
  logfilesem = sem_open("LOGFILE", O_CREAT | O_EXCL, 0700, 1);
  int shmid = shmget(IPC_PRIVATE, sizeof(Transaction) * transaction_pool_size,
                     IPC_CREAT | 0700);
  // create miner process and its threads
  if (fork() == 0) {
    initminers(num_miners);
  } else {
    wait(NULL);
    terminate(shmid);
  }
}

void processFile(int **arr, char *filename) {
  FILE *config;
  char buffer[BUFFER_SIZE];
  int found = 0;
  config = fopen(filename, "r");
  if (config == NULL) {
    printf("Error: Could not open file %s\n", filename);
    return;
  }
  for (int i = 0; i < 4; i++) {
    int ii, slen;
    if (fgets(buffer, BUFFER_SIZE - 1, config) == NULL) {
      printf("Error: The file must have 4 lines, error reading line %d\n", i);
      exit(1);
    }
    slen = strlen(buffer);
    buffer[slen] = '\0';
    for (ii = 0; ii < slen && found == 0; ii++) {
      if (ii < slen - 1 && buffer[ii] == '-' && buffer[ii + 1] == ' ') {
        *arr[i] = atoi(buffer + ii + 2);
        if (*arr[i] == 0 && buffer[ii + 2] != '0') {
          printf("Error: Invalid number in line %d\n Please fix the %s file\n",
                 i, filename);
          fclose(config);
          exit(1);
        }
        found = 1;
      }
    }
    if (found == 0) {
      printf("Error: Invalid number in line %d\n Please fix the %s file\n", i,
             filename);
      fclose(config);
      exit(1);
    }
    found = 0;
  }

  fclose(config);
  return;
}

void terminate(int shmid) {
  sem_close(logfilesem);
  sem_unlink("LOGFILE");
  shmctl(shmid, IPC_RMID, NULL);
}

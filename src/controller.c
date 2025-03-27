#include "controller.h"
#include "miner.h"
#include "statistics.h"
#include "transaction.h"
#include <fcntl.h>
#include <pthread.h>
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
void terminate();
void processFile(unsigned int **arr, char *filename);

sem_t *logfilesem = NULL;
pthread_mutex_t transactionidmutex = PTHREAD_MUTEX_INITIALIZER;
int shmid, shmidtransactions;
int *transactionid;
transaction *transactions_pool;

void quiter(int sig) {
  terminate();
  printf("fuh this shi bruh : %d\n", sig);
  exit(0);
}

void init() {
  signal(SIGINT, quiter);

  char *filename = "config.cfg";
  unsigned int num_miners = 0, pool_size = 0, blockchain_blocks = 0,
               transaction_pool_size = 0;
  unsigned int *ar[] = {&num_miners, &blockchain_blocks, &transaction_pool_size,
                        &pool_size};
  // read file info
  processFile(ar, filename);
  printf("num_miners : %d  pool_size : %d blockchain_blocks : %d "
         "transaction_pool_size : %d\n",
         num_miners, pool_size, blockchain_blocks, transaction_pool_size);

  // semaphores
  sem_unlink("LOGFILE");
  sem_unlink("TRANSACTIONID");
  logfilesem = sem_open("LOGFILE", O_CREAT | O_EXCL, 0700, 1);

  shmid = shmget(IPC_PRIVATE, sizeof(transaction) * transaction_pool_size,
                 IPC_CREAT | 0700);
  transactions_pool = (transaction *)shmat(shmid, NULL, 0);
  shmidtransactions = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0700);
  transactionid = (int *)shmat(shmidtransactions, NULL, 0);
  // create miner process and its threads
  if (fork() == 0) {
    initminers(num_miners);
  } else {
    wait(NULL);
  }
  while (1) {
  }
}

void processFile(unsigned int **arr, char *filename) {
  char *options[] = {"NUM_MINERS -", "BLOCKCHAIN_BLOCKS -",
                     "TRANSACTION_POOL_SIZE -", "POOL_SIZE -"};
  FILE *config;
  char buffer[BUFFER_SIZE];
  int found = 0, ii, slen;
  config = fopen(filename, "r");
  if (config == NULL) {
    printf("Error: Could not open file %s\n", filename);
    return;
  }
  while (fgets(buffer, BUFFER_SIZE - 1, config) != NULL) {
    slen = strlen(buffer);
    buffer[slen] = '\0';
    for (ii = 0; ii < 4 && found == 0; ii++) {
      char *substring = strstr(buffer, options[ii]);
      if (substring != NULL) {
        sscanf(substring + strlen(options[ii]), "%d", arr[ii]);
        found++;
      }
    }
    found--;
  }

  fclose(config);
  return;
}

void terminate() {
  sem_close(logfilesem);
  sem_unlink("LOGFILE");
  sem_unlink("TRANSACTIONID");
  shmdt(transactionid);
  shmdt(transactions_pool);
  shmctl(shmidtransactions, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  printf("terminating\n");
}

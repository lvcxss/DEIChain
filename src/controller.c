#include "controller.h"
#include "deichain.h"
#include "miner.h"
#include "statistics.h"
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
#define BUFFER_SIZE 16

void init();
void terminate();
void write_logfile(char *message, char *typemsg);
Config processFile(char *filename);

// init semaphore for logfile
pthread_mutex_t logfilemutex = PTHREAD_MUTEX_INITIALIZER;
int shmid, shmid_ledger, shmid_transaction_pool_index;
int *index_transaction_pool;
TransactionPool *transactions_pool;
BlockchainLedger *block_ledger;

void quiter(int sig) {
  if (sig == 2) {
    terminate();
    write_logfile("Closing program", "INFO");
    printf("Closing program\n");
    exit(0);
  }
}

void init() {
  signal(SIGINT, quiter);

  transactionid = 0;
  char *filename = "config.cfg";
  // read file info
  Config config = processFile(filename);
  printf("num_miners : %d  pool_size : %d blockchain_blocks : %d "
         "transaction_pool_size : %d\n",
         config.num_miners, config.tx_pool_size, config.blockchain_blocks,
         config.transactions_per_block);
  char configmsg[256];
  sprintf(configmsg,
          "Configuration loaded :\n   Number of Miners: %d \n   Transaction "
          "pool size: %d\n   Maximum number of blocks in the block chain: %d\n "
          "  Number of Transacions in a block: %d\n",
          config.num_miners, config.tx_pool_size, config.blockchain_blocks,
          config.transactions_per_block);
  write_logfile(configmsg, "INFO");
  // init shared memory
  // transaction pool
  shmid = shmget(IPC_PRIVATE,
                 sizeof(TransactionPool) +
                     sizeof(Transaction) * config.transactions_per_block,
                 IPC_CREAT | 0700);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  transactions_pool = (TransactionPool *)shmat(shmid, NULL, 0);
  if (transactions_pool == (void *)-1) {
    perror("shmat");
    exit(1);
  }
  // index transaction pool
  shmid_transaction_pool_index =
      shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0700);
  index_transaction_pool = (int *)shmat(shmid_transaction_pool_index, NULL, 0);
  *index_transaction_pool = 0;

  // block ledger
  shmid_ledger = shmget(
      IPC_PRIVATE,
      sizeof(BlockchainLedger) + sizeof(Block) * config.blockchain_blocks +
          sizeof(Transaction) * config.transactions_per_block *
              config.blockchain_blocks,
      IPC_CREAT | 0700);
  if (shmid_ledger == -1) {
    write_logfile("Error creating shared memory for block ledger (shmget)",
                  "ERROR");
    perror("shmget");
    exit(1);
  }
  block_ledger = (BlockchainLedger *)shmat(shmid, NULL, 0);
  if (block_ledger == (void *)-1) {
    write_logfile("Error attaching shared memory for block ledger (shmat)",
                  "ERROR");
    perror("shmat");
    exit(1);
  }

  // create miner process and its threads
  if (fork() == 0) {
    write_logfile("Miner process created", "INIT");
    printf("Miner process created\n");
    initminers(config.num_miners);
  } else {
    if (fork() == 0) {
      write_logfile("Statistics process created", "INIT");
      printf("Statistics process created\n");

      print_statistics();
    } else {
      wait(NULL);
      while (1) {
      }
    }
  }
}

Config processFile(char *filename) {
  // arr to optimize
  Config cfg;
  FILE *config;
  char buffer[BUFFER_SIZE];
  int found = 0, slen;
  write_logfile("Started reading configuration file", "INIT");
  config = fopen(filename, "r");
  if (config == NULL) {
    write_logfile("Could not open file", "ERROR");
    printf("Error: Could not open file %s\n", filename);
    exit(1);
  }
  int num;
  while (fgets(buffer, BUFFER_SIZE - 1, config)) {
    if (found >= 4) {
      write_logfile("O ficheiro tem mais de 4 linhas", "ERROR");
      printf("O ficheiro tem mais de 4 linhas por favor corrija.\n");
      exit(1);
    }
    slen = strlen(buffer);
    buffer[slen] = '\0';
    num = atoi(buffer);
    if (num == 0 && buffer[0] != '0') {
      printf("Erro ao ler o ficheiro, deveria ter 4 linhas separadas com "
             "inteiros.\n");
      exit(1);
    }
    switch (found) {
    case 0:
      cfg.num_miners = num;
      break;
    case 1:
      cfg.tx_pool_size = num;
      break;
    case 2:
      cfg.blockchain_blocks = num;
      break;
    case 3:
      cfg.transactions_per_block = num;
      break;
    }
    found++;
  }
  fclose(config);
  return cfg;
}

void write_logfile(char *message, char *typemsg) {
  pthread_mutex_lock(&logfilemutex);
  FILE *logfile = fopen("logfile.txt", "a");
  if (logfile == NULL) {
    printf("Error: Could not open file logfile.txt\n");
    exit(1);
  }
  fprintf(logfile, "[%s] (%ld) %s\n", typemsg, time(NULL), message);
  fclose(logfile);
  pthread_mutex_unlock(&logfilemutex);
}

void terminate() {
  write_logfile("Destroying mutex", "TERMINATE");
  pthread_mutex_destroy(&logfilemutex);
  write_logfile("Detaching shared memory", "TERMINATE");
  shmdt(transactions_pool);
  shmdt(index_transaction_pool);
  shmdt(block_ledger);
  write_logfile("Removing shared memory", "TERMINATE");
  shmctl(shmid_transaction_pool_index, IPC_RMID, NULL);
  shmctl(shmid_ledger, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  write_logfile("Finished terminating", "TERMINATE");
  printf("Finished terminating\n");
}

// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "controller.h"
#include "deichain.h"
#include "miner.h"
#include "statistics.h"
#include "transaction.h"
#include "validator.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#define BUFFER_SIZE 16
#define DEBUG
#define PIPE_NAME "VALIDATOR_INPUT"
void init();
void terminate();
int write_logfile(char *message, char *typemsg);
int create_miner_process(int nt);
int create_validator_process();
int create_statistics_process();
int create_pipe();
Config processFile(char *filename);

int shmid, shmid_ledger;
TransactionPool *transactions_pool_info;
Transaction *transactions_pool;
Config config;
BlockchainLedger *block_ledger;

void quiter(int sig) {
  if (sig == 2) {
    TransactionPoolEntry *transactions =
        (TransactionPoolEntry *)((char *)transactions_pool_info +
                                 sizeof(TransactionPool));

    printf("Transactions : ");
    sem_wait(transactions_pool_info->transaction_pool_sem);
    for (int i = 0; i < transactions_pool_info->id; i++) {
      print_transaction(transactions[i]);
    }
    sem_post(transactions_pool_info->transaction_pool_sem);
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
  config = processFile(filename);

#ifdef DEBUG
  char configmsg[256];
  sprintf(configmsg,
          "Configuration loaded :\n   Number of Miners: %d \n   Transaction "
          "pool size: %d\n   Maximum number of blocks in the block chain: %d\n "
          "  Number of Transacions in a block: %d",
          config.num_miners, config.tx_pool_size, config.blockchain_blocks,
          config.transactions_per_block);
  write_logfile(configmsg, "INFO");
  printf("%s\n", configmsg);
#endif
  // init shared memory
  // transaction pool
  key_t key = ftok("config.cfg", 65);
  shmid = shmget(key,
                 sizeof(TransactionPool) + sizeof(TransactionPoolEntry) *
                                               config.transactions_per_block,
                 IPC_CREAT | 0700);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  transactions_pool_info = (TransactionPool *)shmat(shmid, NULL, 0);
  if (transactions_pool_info == (void *)-1) {
    perror("shmat");
    exit(1);
  }
  // index transaction pool (unused by now but the logic would be something like
  // fill the transaction pool and iterate throw it from
  // 0-TRANSACTION_POOL_SIZE, and repeat => index= ++index %
  // TRANSACTION_POOL_SIZE)

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
  block_ledger = (BlockchainLedger *)shmat(shmid_ledger, NULL, 0);
  if (block_ledger == (void *)-1) {
    write_logfile("Error attaching shared memory for block ledger (shmat)",
                  "ERROR");
    perror("shmat");
    exit(1);
  }

  memset(transactions_pool_info, 0,
         sizeof(TransactionPool) +
             sizeof(TransactionPoolEntry) * config.transactions_per_block);
  memset(block_ledger, 0,
         sizeof(*block_ledger) + sizeof(Block) * config.blockchain_blocks +
             sizeof(Transaction) * config.transactions_per_block *
                 config.blockchain_blocks);
  transactions_pool_info->id = 0;
  transactions_pool_info->transaction_pool_sem =
      sem_open("/transaction_pool_sem", O_CREAT, 0666, 1);
  if (transactions_pool_info->transaction_pool_sem == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }
  transactions_pool_info->max_size = config.tx_pool_size;

  // DONT REMOVE THIS LINE !!!
  // problem : whenever a child process is created, the son gets a copy of the
  // parent's stdout
  fflush(stdout);
  // create processes
  if (create_miner_process(config.num_miners) == 1) {
    terminate();
  }
  if (create_statistics_process() == 1) {
    terminate();
  }
  if (create_validator_process() == 1) {
    terminate();
  }
  if (create_pipe() == 1) {
    terminate();
  }
  int i;
  for (i = 0; i < 3; i++)
    wait(NULL);
  while (1) {
  }
}

int create_pipe() {
  if ((mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
    perror("Cannot create pie: ");
    return 1;
  }
  return 0;
}

// function for creating the miner process
int create_miner_process(int nt) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    printf("Error creating miner process\n");
    write_logfile("Error creating miner process", "ERROR");
    return 1;
  } else if (pid == 0) {
    write_logfile("Miner process created", "INIT");
    printf("Miner process created\n");
    // function that starts all miner threads
    initminers(nt);
    write_logfile("Miner process terminated", "TERMINATE");
    exit(0);
  }
  return 0;
}

// function for creating the statistics process
int create_statistics_process() {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    printf("Error creating statistics process\n");
    write_logfile("Error creating statistics process", "ERROR");
    return 1;
  } else if (pid == 0) {
    write_logfile("Statistics process created", "INIT");
    printf("Statistics process created\n");
    print_statistics();
    write_logfile("Statistics process terminated", "TERMINATE");
    exit(0);
  }
  return 0;
}

// function to create the validator process
int create_validator_process() {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    write_logfile("Error creating validator process", "ERROR");
    return 1;
  } else if (pid == 0) {
    write_logfile("Validator process created", "INIT");
    printf("Validator process created\n");
    validator();
    write_logfile("Validator process terminated", "TERMINATE");
    exit(0);
  }
  return 0;
}

// function to process the configuration file
Config processFile(char *filename) {
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
    // if file has >4 lines, quit
    if (found >= 4) {
      write_logfile("O ficheiro tem mais de 4 linhas", "ERROR");
      printf("O ficheiro tem mais de 4 linhas por favor corrija.\n");
      exit(1);
    }
    slen = strlen(buffer);
    buffer[slen] = '\0';
    num = atoi(buffer);
    // check if it is a number
    if (num == 0 && buffer[0] != '0') {
      printf("Erro ao ler o ficheiro, deveria ter 4 linhas separadas com "
             "inteiros.\n");
      exit(1);
    }
    // the numbers must be valid
    if (num <= 0) {
      write_logfile("O numero presente na configuracao e muito pequeno por "
                    "favor corrija.",
                    "ERROR");
      printf("O numero presente na configuracao e muito pequeno por favor "
             "corrija.\n");
      exit(1);
    }
    // filling the config struct
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
  // the file must have 4 lines
  if (found != 4) {
    write_logfile("O ficheiro tem menos de 4 linhas", "ERROR");
    printf("Erro: O ficheiro de configuração tem menos de 4 linhas.\n");
    exit(1);
  }
  fclose(config);
  return cfg;
}

void terminate() {
  write_logfile("Destroying sem", "TERMINATE");
  sem_close(transactions_pool_info->transaction_pool_sem);
  sem_unlink("/transaction_pool_sem");
  write_logfile("Destroying mutex", "TERMINATE");
  // unlink pipe
  unlink(PIPE_NAME);
  // destroy mutex
  // NOTE: professor said the pthread_mutex_destroy is enough
  pthread_mutex_destroy(&logfilemutex);
  write_logfile("Detaching shared memory", "TERMINATE");
  // detach shared memory
  if (shmdt(transactions_pool_info) == -1)
    perror("shmdt transactions_pool");
  if (shmdt(block_ledger) == -1)
    perror("shmdt block_ledger");
  write_logfile("Removing shared memory", "TERMINATE");
  // remove shared memory (flag : IPC_RMID)
  if (shmctl(shmid_ledger, IPC_RMID, NULL) == -1)
    perror("shmctl block_ledger");
  if (shmctl(shmid, IPC_RMID, NULL) == -1)
    perror("shmctl transactions_pool");
  write_logfile("Finished terminating", "TERMINATE");
  printf("\nFinished terminating\n");
}

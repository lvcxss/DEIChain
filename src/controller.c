// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "controller.h"
#include "deichain.h"
#include "miner.h"
#include "pow.h"
#include "statistics.h"
#include "validator.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#define BUFFER_SIZE 16
#define DEBUG 0
#define PIPE_NAME "VALIDATOR_INPUT"

void init();
void terminate();
int write_logfile(char *message, char *typemsg);
int create_miner_process(int nt);
int create_validator_process();
int create_statistics_process();
int create_pipe();
Config processFile(char *filename);

int pids[3];
int mainpid;
pthread_condattr_t cattr, cattrvc, cattrct;
pthread_mutexattr_t mattr, mattrvc, mattrct;
sem_t *log_file_mutex;

int shmid, shmidledger;
TransactionPool *transactions_pool;
TransactionPoolEntry *transactions;
Config config;
BlockchainLedger *block_ledger;

void sighandler(int sig) {
  if (sig == SIGINT) {
    write_logfile("SIGINT received", "Controller");

    transactions_pool->max_size = 0;
    for (int i = 0; i < 3; i++) {
      pthread_cond_signal(&transactions_pool->cond_vc);
      if (pids[i] > 0) {
        kill(pids[i], SIGTERM);
      }
    }

    for (int i = 0; i < 3; i++) {
      if (pids[i] > 0) {
        int status;
        waitpid(pids[i], &status, 0);
      }
    }
    if (getpid() == mainpid) {
      if (block_ledger->num_blocks > 0) {
        printf("Dumping block ledger : \n");
        Block *blocks =
            (Block *)((char *)block_ledger + sizeof(BlockchainLedger));
        for (int i = 0; i < block_ledger->num_blocks; i++) {
          Block *b =
              (Block *)((char *)blocks +
                        (sizeof(Transaction) * config.transactions_per_block +
                         sizeof(Block)) *
                            i);
          Transaction *blcktrans = (Transaction *)((char *)b + sizeof(Block));
          printf("Block %d : %d\n", i, b->nonce);
          show_block(b, blcktrans);
        }

        write_ledger_logfile();
      }
      terminate();
      write_logfile("Closing program", "INFO");
      printf("Closing program\n");
      exit(0);
    }
    if (sig == SIGUSR1) {
      sem_wait(block_ledger->ledger_sem);
      Block *blocks =
          (Block *)((char *)block_ledger + sizeof(BlockchainLedger));
      for (int i = 0; i < block_ledger->num_blocks; i++) {
        blocks = blocks + (sizeof(BlockchainLedger) +
                           sizeof(Transaction) * config.transactions_per_block);
      }
      sem_post(block_ledger->ledger_sem);
    }
  }
}
void init() {
  mainpid = getpid();
  signal(SIGCHLD, SIG_IGN);

  struct sigaction sa_int;
  sa_int.sa_handler = sighandler;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = 0;
  if (sigaction(SIGINT, &sa_int, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  // Configuração do sigaction para SIGUSR1
  struct sigaction sa_usr1;
  sa_usr1.sa_handler = sighandler;
  sigemptyset(&sa_usr1.sa_mask);
  sa_usr1.sa_flags = 0;
  if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  transactionid = 0;
  sem_unlink("/log_file_mutex");
  log_file_mutex = sem_open("/log_file_mutex", O_CREAT | O_EXCL, 0700, 1);
  if (log_file_mutex == SEM_FAILED)
    perror("Creating semaphore for log file");

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
  key_t key_ledger = ftok("config.cfg", 66);
  shmid = shmget(key,
                 sizeof(TransactionPool) + sizeof(TransactionPoolEntry) *
                                               config.transactions_per_block,
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

  // index transaction pool (unused by now but the logic would be something
  // like fill the transaction pool and iterate throw it from
  // 0-TRANSACTION_POOL_SIZE, and repeat => index= ++index %
  // TRANSACTION_POOL_SIZE)
  // cv for miners
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  pthread_condattr_init(&cattr);
  pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

  // block ledger
  shmidledger = shmget(key_ledger,
                       sizeof(BlockchainLedger) +
                           sizeof(Block) * config.blockchain_blocks +
                           sizeof(Transaction) * config.transactions_per_block *
                               config.blockchain_blocks,
                       IPC_CREAT | 0700);
  if (shmidledger == -1) {
    write_logfile("Error creating shared memory for block ledger (shmget)",
                  "ERROR");
    perror("shmget");
    exit(1);
  }
  block_ledger = (BlockchainLedger *)shmat(shmidledger, NULL, 0);
  if (block_ledger == (void *)-1) {
    write_logfile("Error attaching shared memory for block ledger (shmat)",
                  "ERROR");
    perror("shmat");
    exit(1);
  }

  memset(transactions_pool, 0,
         sizeof(TransactionPool) +
             sizeof(TransactionPoolEntry) * config.transactions_per_block);
  memset(block_ledger, 0,
         sizeof(BlockchainLedger) + sizeof(Block) * config.blockchain_blocks +
             sizeof(Transaction) * config.transactions_per_block *
                 config.blockchain_blocks);

  transactions = (TransactionPoolEntry *)((char *)transactions_pool +
                                          sizeof(TransactionPool));

  sem_unlink("/sem_min_tx");
  transactions_pool->sem_min_tx = sem_open("/sem_min_tx", O_CREAT, 0666, 0);
  if (transactions_pool->sem_min_tx == SEM_FAILED) {
    perror("sem_open /sem_min_tx");
    exit(1);
  }
  transactions_pool->atual = 0;
  sem_unlink("/tp_access_pool");
  transactions_pool->tp_access_pool =
      sem_open("/tp_access_pool", O_CREAT, 0666, 2);
  if (transactions_pool->tp_access_pool == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }
  sem_unlink("/ledger_sem");
  block_ledger->ledger_sem = sem_open("/ledger_sem", O_CREAT, 0666, 1);
  if (block_ledger->ledger_sem == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }
  block_ledger->num_blocks = 0;
  block_ledger->total_blocks = config.blockchain_blocks;
  strcpy(block_ledger->hash_atual, INITIAL_HASH);
  transactions_pool->max_size = config.tx_pool_size;
  transactions_pool->transactions_block = config.transactions_per_block;

  // cv usada para o validator controller
  pthread_mutexattr_init(&mattrvc);
  pthread_mutexattr_setpshared(&mattrvc, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&transactions_pool->mt_vc, &mattrvc);
  pthread_condattr_init(&cattrvc);
  pthread_condattr_setpshared(&cattrvc, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&transactions_pool->cond_vc, &cattrvc);

  // cv usado para o controller sair quando a blockledger estiver cheia
  pthread_mutexattr_init(&mattrct);
  pthread_mutexattr_setpshared(&mattrct, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&block_ledger->mt_ct, &mattrct);
  pthread_condattr_init(&cattrct);
  pthread_condattr_setpshared(&cattrct, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&block_ledger->cond_ct, &cattrct);

  // DONT REMOVE THIS LINE !!!
  // problem : whenever a child process is created, the son gets a copy of
  // the parent's stdout
  fflush(stdout);
  // create processesm
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
  // creation of queue
  key_t stats_key = ftok("config.cfg", 67);
  int stats_qid = msgget(stats_key, IPC_CREAT | 0666);
  if (stats_qid == -1) {
    perror("msgget stats");
    exit(1);
  }
  while (block_ledger->num_blocks < block_ledger->total_blocks) {
    pthread_mutex_lock(&block_ledger->mt_ct);
    pthread_cond_wait(&block_ledger->cond_ct, &block_ledger->mt_ct);
    pthread_mutex_unlock(&block_ledger->mt_ct);
  }
  printf("Block ledger cheia, a terminar\n");
  sighandler(SIGINT);
}

// creating named pipe
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
    write_logfile("Error creating miner process", "Controller");
    return 1;
  } else if (pid == 0) {
    signal(SIGINT, SIG_IGN);
    write_logfile("Miner process created", "Controller");
    // function that starts all miner threads
    initminers(nt);
    write_logfile("Miner process terminated", "Controller");
    exit(0);
  }
  pids[0] = pid;

  return 0;
}

// function for creating the statistics process
int create_statistics_process() {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    printf("Error creating statistics process\n");
    write_logfile("Error creating statistics process", "Statistics");
    return 1;
  } else if (pid == 0) {
    signal(SIGINT, SIG_IGN);
    write_logfile("Statistics process created", "Statistics");
    printf("Para verificar estatisticas envie SIGUSR1 para : %d\n", getpid());
    print_statistics();
    write_logfile("Statistics process terminated", "Statistics");
    exit(0);
  }
  pids[1] = pid;
  return 0;
}

// function to create the validator process
int create_validator_process() {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    write_logfile("Error creating validator controller process",
                  "Validator Controller");
    return 1;
  } else if (pid == 0) {
    signal(SIGINT, SIG_IGN);
    write_logfile("Validator controller process created",
                  "Validator Controller");
    validator_controller();
    write_logfile("Validator process terminated", "Validator Controller");
    exit(0);
  }
  pids[2] = pid;
  return 0;
}

// function to process the configuration file
Config processFile(char *filename) {
  Config cfg;
  FILE *config;
  char buffer[BUFFER_SIZE];
  int found = 0, slen;
  write_logfile("Started reading configuration file", "Controller");
  config = fopen(filename, "r");
  if (config == NULL) {
    write_logfile("Could not open file", "Controller");
    printf("Error: Could not open file %s\n", filename);
    exit(1);
  }
  int num;
  while (fgets(buffer, BUFFER_SIZE - 1, config)) {
    // if file has >4 lines, quit
    if (found >= 4) {
      write_logfile("O ficheiro tem mais de 4 linhas", "Controller");
      printf("O ficheiro tem mais de 4 linhas por favor corrija.\n");
      fclose(config);
      exit(1);
    }
    slen = strlen(buffer);
    buffer[slen] = '\0';
    num = atoi(buffer);
    // check if it is a number
    if (num == 0 && buffer[0] != '0') {
      printf("Erro ao ler o ficheiro, deveria ter 4 linhas separadas com "
             "inteiros.\n");
      fclose(config);
      exit(1);
    }
    // the numbers must be valid
    if (num <= 0) {
      write_logfile("O numero presente na configuracao e muito pequeno por "
                    "favor corrija.",
                    "Controller");
      printf("O numero presente na configuracao e muito pequeno por favor "
             "corrija.\n");
      fclose(config);
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
    write_logfile("O ficheiro tem menos de 4 linhas", "Controller");
    printf("Erro: O ficheiro de configuração tem menos de 4 linhas.\n");
    fclose(config);
    exit(1);
  }
  fclose(config);
  return cfg;
}

void terminate() {
  write_logfile("Destroying named semaphores", "Controller");

  sem_close(transactions_pool->sem_min_tx);
  sem_unlink("/sem_min_tx");

  sem_close(transactions_pool->tp_access_pool);
  sem_unlink("/tp_access_pool");

  sem_close(block_ledger->ledger_sem);
  sem_unlink("/ledger_sem");

  sem_close(log_file_mutex);
  sem_unlink("/log_file_mutex");
  log_file_mutex = NULL;
  write_logfile("Destroying mutex", "Controller");
  unlink(PIPE_NAME);

  write_logfile("Detaching shared memory", "Controller");
  shmdt(transactions_pool);
  shmdt(block_ledger);

  write_logfile("Removing shared memory", "Controller");
  shmctl(shmidledger, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);

  write_logfile("Finished terminating", "Controller");
  printf("\nFinished terminating\n");
}

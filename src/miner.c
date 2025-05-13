// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva

#include "deichain.h"
#include "pow.h"
#include <fcntl.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
int cnt = 0;

PoWResult result = {"", 0, 0, 0};
int aloc = 0;
int exited = 0;
volatile sig_atomic_t miner_should_exit = 0;

void handle_sigterm(int signum) {
  if (signum == SIGTERM || signum == SIGINT) {
    miner_should_exit = 1;
    pthread_cond_broadcast(&transactions_pool->cond_min);
  }
}

void *mine(void *idp) {
  Block *new_block = NULL;
  int pipe_fd = -1;
  unsigned int tid = *((int *)idp);

  while (!miner_should_exit) {
    new_block = malloc(get_transaction_block_size());
    if (!new_block) {
      perror("Erro ao alocar memoria");
      goto cleanup;
    }

    sem_wait(block_ledger->ledger_sem);
    snprintf(new_block->block_id, TX_ID_LEN, "BLOCK-%lu-%d", (unsigned long)tid,
             block_ledger->num_blocks);
    memcpy(new_block->previous_hash, block_ledger->hash_atual, HASH_SIZE);
    sem_post(block_ledger->ledger_sem);

    unsigned int numm = 0;
    Transaction *transactions_block =
        (Transaction *)((char *)new_block + sizeof(Block));
    unsigned int it = 0;
    int samet = 0;
    do {
      numm = 0;
      samet = 0;
      unsigned int s = (transactions_pool->atual > transactions_pool->max_size)
                           ? transactions_pool->max_size
                           : transactions_pool->atual;

      if (miner_should_exit) {
        break;
      }
      for (it = 0; it < s; it++) {
        if (numm >= config.transactions_per_block) {
          break;
        }
        if (transactions[it].occupied) {
          memcpy(&transactions_block[numm], &transactions[it].transaction,
                 sizeof(Transaction));
          numm++;
        }
      }
      if (numm < config.transactions_per_block &&
          transactions_pool->available < config.transactions_per_block) {
        pthread_mutex_lock(&transactions_pool->mt_min);
        pthread_cond_wait(&transactions_pool->cond_min,
                          &transactions_pool->mt_min);
        pthread_mutex_unlock(&transactions_pool->mt_min);
      }

      if (numm >= config.transactions_per_block) {
        for (unsigned int tit = 0; tit < config.transactions_per_block; tit++) {
          for (unsigned int tit2 = tit; tit2 < config.transactions_per_block;
               tit2++) {
            if (strcmp(transactions_block[tit].transaction_id,
                       transactions_block[tit2].transaction_id) == 0) {
              samet = 1;
            }
          }
        }
      }

    } while (numm < config.transactions_per_block && !miner_should_exit &&
             !samet);

    if (miner_should_exit)
      break;

    /* 4) proof-of-work */
    do {
      result = proof_of_work(new_block);
      new_block->timestamp = time(NULL);
    } while (result.error == 1 && !miner_should_exit);

    if (miner_should_exit)
      break;

    pipe_fd = open("VALIDATOR_INPUT", O_WRONLY);
    if (pipe_fd < 0) {
      perror("Erro ao abrir o pipe");
      goto cleanup;
    }
    write(pipe_fd, new_block, get_transaction_block_size());
    write(pipe_fd, &result, sizeof(result));
    close(pipe_fd);
    pipe_fd = -1;

    /* 6) liberta memória e repete */
    free(new_block);
    new_block = NULL;
  }

cleanup:
  if (pipe_fd >= 0)
    close(pipe_fd);
  if (new_block)
    free(new_block);
  return NULL;
}

void initminers(int num) {
  struct sigaction sa;
  sa.sa_handler = handle_sigterm;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  pthread_t thread_miners[num];
  int id[num];
  int i = 0;
  char msg[256];

  key_t key = ftok("config.cfg", 65);
  key_t key_ledger = ftok("config.cfg", 66);
  int shmid = shmget(key, 0, 0666);
  transactions_pool = shmat(shmid, NULL, 0);
  transactions = (TransactionPoolEntry *)((char *)transactions_pool +
                                          sizeof(TransactionPool));

  int shmid_ledger = shmget(key_ledger, 0, 0666);
  block_ledger = shmat(shmid_ledger, NULL, 0);

  // creates threads
  for (i = 0; i < num; i++) {
    id[i] = i + 1;
    pthread_create(&thread_miners[i], NULL, mine, &id[i]);
    sprintf(msg, "Miner %d created", id[i]);
    write_logfile(msg, "INIT");
  }

  // waits for threads
  for (i = 0; i < num; i++) {
    pthread_cond_signal(&transactions_pool->cond_min);
    pthread_join(thread_miners[i], NULL);
    char msg[256];
    sprintf(msg, "Miner %d finished", id[i]);
    write_logfile(msg, "INFO");
    printf("%s\n", msg);
  }
  pthread_cond_destroy(&transactions_pool->cond_min);
  pthread_mutex_destroy(&transactions_pool->mt_min);
  sem_close(transactions_pool->transaction_pool_sem);

  sem_close(transactions_pool->tp_access_pool);

  sem_close(block_ledger->ledger_sem);

  sem_close(log_file_mutex);
  log_file_mutex = NULL;
  shmdt(transactions_pool);
  shmdt(block_ledger);
  shmctl(shmidledger, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  printf("Exiting miners\n");
  exit(0);
}

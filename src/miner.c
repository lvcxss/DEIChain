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
volatile sig_atomic_t miner_should_exit = 0;

void handle_sigterm(int signum) {
  if (signum == SIGTERM || signum == SIGINT) {
    miner_should_exit = 1;
    pthread_cond_broadcast(&transactions_pool->cond_min);
  }
}

void *mine(void *idp) {
  int aloc = 0;
  Block *new_block;
  while (!miner_should_exit) {
    new_block = malloc(get_transaction_block_size());
    aloc = 1;
    if (!new_block) {
      printf("Erro ao alocar memoria");
    }
    unsigned int tid = (*((int *)idp));
    sem_wait(block_ledger->ledger_sem);
    snprintf(new_block->block_id, TX_ID_LEN, "BLOCK-%lu-%d", (unsigned long)tid,
             block_ledger->num_blocks);
    memcpy(new_block->previous_hash, block_ledger->hash_atual, HASH_SIZE);
#ifdef DEBUG
    printf("previous_hash %s\n", new_block->previous_hash);
#endif
    sem_post(block_ledger->ledger_sem);
    unsigned int i, numm = 0;
    int lasti = 0;
    Transaction *transactions_block =
        (Transaction *)((char *)new_block + sizeof(Block));
    while (numm < config.transactions_per_block) {
      unsigned int s = (transactions_pool->atual > transactions_pool->max_size)
                           ? transactions_pool->max_size
                           : transactions_pool->atual;

      if (s == config.tx_pool_size) {
        lasti = 0;
        numm = 0;
      }
      for (i = lasti; i < s; i++) {
        if (numm >= config.transactions_per_block) {
          break;
        }
        if (transactions[i].occupied) {
          memcpy(&transactions_block[numm], &transactions[i].transaction,
                 sizeof(Transaction));
          numm++;
        }
      }
      lasti = i;
      if (numm < config.transactions_per_block) {
        pthread_mutex_lock(&transactions_pool->mt_min);
#ifdef DEBUG
        printf("Miner %d waiting for transactions\n", *((int *)idp));
#endif
        pthread_cond_wait(&transactions_pool->cond_min,
                          &transactions_pool->mt_min);

        if (miner_should_exit) {
          pthread_mutex_unlock(&transactions_pool->mt_min);
          return NULL;
        }

        pthread_mutex_unlock(&transactions_pool->mt_min);
      }
    }
    transactions_block = (Transaction *)((char *)new_block + sizeof(Block));
    int max_reward = 0;
    for (unsigned int i = 0; i < config.transactions_per_block; i++)
      if (transactions_block[i].reward > max_reward)
        max_reward = transactions_block[i].reward;
    PoWResult result;
    do {
      result = proof_of_work(new_block);
      new_block->timestamp = time(NULL);
    } while (result.error == 1);
#ifdef DEBUG
    printf("Enviar por pipe\n");
#endif
    int pipe_fd = open("VALIDATOR_INPUT", O_WRONLY);

    if (pipe_fd == -1) {
      perror("Erro ao abrir o pipe");
      return NULL;
    }
    write(pipe_fd, new_block, get_transaction_block_size());

    close(pipe_fd);
    free(new_block);
    aloc = 0;
  }
#ifdef DEBUG
  if (aloc) {
    free(new_block);
  }
  printf("Miner %d finished\n", *((int *)idp));
#endif
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
  int i;
  char msg[256];

  key_t key = ftok("config.cfg", 65);
  key_t key_ledger = ftok("config.cfg", 66);
  int shmid = shmget(key, 0, 0666);
  transactions_pool = shmat(shmid, NULL, 0);
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
    pthread_join(thread_miners[i], NULL);
    char msg[256];
    sprintf(msg, "Miner %d finished", id[i]);
    write_logfile(msg, "INFO");
    printf("%s\n", msg);
  }
  pthread_cond_destroy(&transactions_pool->cond_min);
  pthread_mutex_destroy(&transactions_pool->mt_min);
  exit(0);
}

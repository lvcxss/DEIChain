
#include "deichain.h"
#include "pow.h"
#include <fcntl.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <semaphore.h>
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
volatile sig_atomic_t miner_should_exit = 0;

void handle_sigterm(int signum) {
  if (signum == SIGTERM || signum == SIGINT) {
    miner_should_exit = 1;
    for (unsigned int i = 0; i < config.num_miners; i++) {
      sem_post(transactions_pool->sem_min_tx);
    }
  }
}

void *mine(void *idp) {
  unsigned int tid = *((int *)idp);
  Block *new_block;
  int pipe_fd;
  srand(time(NULL) ^ (uintptr_t)pthread_self());

  while (!miner_should_exit) {
    new_block = malloc(get_transaction_block_size());
    if (!new_block) {
      perror("Erro ao alocar memoria");
      break;
    }

    sem_wait(block_ledger->ledger_sem);
    snprintf(new_block->block_id, TX_ID_LEN, "BLOCK-%lu-%d", (unsigned long)tid,
             rand());
    memcpy(new_block->previous_hash, block_ledger->hash_atual, HASH_SIZE);
    sem_post(block_ledger->ledger_sem);

    Transaction *transactions_block =
        (Transaction *)((char *)new_block + sizeof(Block));

    // --- Seleção aleatória de transações ---
    sem_wait(transactions_pool->tp_access_pool);

    unsigned int limit =
        (transactions_pool->atual > transactions_pool->max_size)
            ? transactions_pool->max_size
            : transactions_pool->atual;
    unsigned int occ = 0;
    for (unsigned int i = 0; i < limit; i++) {
      if (transactions[i].occupied)
        occ++;
    }
    // caso nao haja transacoes suficientes espera
    if (occ < config.transactions_per_block) {
      usleep(500000);
      sem_post(transactions_pool->tp_access_pool);
      sem_wait(transactions_pool->sem_min_tx);
      free(new_block);
      continue;
    }

    unsigned int indices[occ];
    unsigned int k = 0;
    for (unsigned int i = 0; i < limit; i++) {
      if (transactions[i].occupied) {
        indices[k++] = i;
      }
    }

    for (unsigned int i = occ - 1; i > 0; i--) {
      unsigned int j = rand() % (i + 1);
      unsigned int tmp = indices[i];
      indices[i] = indices[j];
      indices[j] = tmp;
    }

    for (unsigned int t = 0; t < config.transactions_per_block; t++) {
      unsigned int idx = indices[t];
      memcpy(&transactions_block[t], &transactions[idx].transaction,
             sizeof(Transaction));
    }

    // sinaliza se ainda restam transactionss
    if (occ - config.transactions_per_block >= config.transactions_per_block) {
      sem_post(transactions_pool->sem_min_tx);
    }
    sem_post(transactions_pool->tp_access_pool);

    do {
      result = proof_of_work(new_block);
      new_block->timestamp = time(NULL);
    } while (result.error == 1 && !miner_should_exit);

    if (miner_should_exit) {
      free(new_block);
      break;
    }
    char st[64];
    sprintf(st, "Miner %d acabou de escrever um bloco", tid);
    write_logfile(st, "Miner");
    pipe_fd = open("VALIDATOR_INPUT", O_WRONLY);
    if (pipe_fd < 0) {
      perror("Erro ao abrir o pipe");
      free(new_block);
      break;
    }
    write(pipe_fd, new_block, get_transaction_block_size());
    write(pipe_fd, &result, sizeof(result));
    close(pipe_fd);
    free(new_block);
  }

  return NULL;
}

void initminers(int num) {
  struct sigaction sa;
  sa.sa_handler = handle_sigterm;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  pthread_t thread_miners[num];
  int id[num];
  for (int i = 0; i < num; i++) {
    id[i] = i + 1;
    pthread_create(&thread_miners[i], NULL, mine, &id[i]);
    char msg[128];
    sprintf(msg, "Miner %d created", id[i]);
    write_logfile(msg, "Miner");
  }

  for (int i = 0; i < num; i++) {
    pthread_join(thread_miners[i], NULL);
    char msg[128];
    sprintf(msg, "Miner %d finished", id[i]);
    write_logfile(msg, "Miner");
    printf("%s\n", msg);
  }

  log_file_mutex = NULL;

  printf("Exiting miners\n");
  exit(0);
}

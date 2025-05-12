// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include "pow.h"
#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
// verifying pow
// verifying hash
//  ageing transactions
Transaction *tx;
int stats_qid;
volatile sig_atomic_t should_exit = 0;

void handle_term(int sig) {
  if (sig) {
    should_exit = 1;
  }
}

int sum_rewards(Transaction *tx, unsigned int num) {
  int sum = 0;
  for (unsigned int i = 0; i < num; i++) {
    sum += tx[i].reward;
  }
  return sum;
}

int validator() {
  struct sigaction sa;
  sa.sa_handler = handle_term;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  key_t key = ftok("config.cfg", 65);
  key_t key_ledger = ftok("config.cfg", 66);
  int shmid = shmget(key, 0, 0666);
  transactions_pool = shmat(shmid, NULL, 0);
  int shmid_ledger = shmget(key_ledger, 0, 0666);
  block_ledger = shmat(shmid_ledger, NULL, 0);
  block_ledger->ledger_sem = sem_open("/ledger_sem", 0);
  if (block_ledger->ledger_sem == SEM_FAILED) {
    perror("sem_open /ledger_sem");
    return 1;
  }

  const size_t block_sz = get_transaction_block_size();
  unsigned char *raw = malloc(block_sz);
  PoWResult *results = malloc(sizeof(PoWResult));

  if (!results) {
    perror("malloc");
    return 1;
  }
  if (!raw) {
    perror("malloc");
    return 1;
  }

  int fd;
  if ((fd = open("VALIDATOR_INPUT", O_RDWR)) < 0) {
    perror("open VALIDATOR_INPUT");
    free(raw);
    return 1;
  }

  while (!should_exit) {
    ssize_t r = read(fd, raw, block_sz);
    ssize_t rr = read(fd, results, sizeof(PoWResult));
    if (r == 0) {
      break;
    } else if (r < 0) {
      if (errno == EINTR)
        continue;
      perror("read");
      break;
    } else if ((size_t)r < block_sz) {
      fprintf(stderr, "Bloco nao foi lido completamente: %zd/%zu bytes\n", r,
              block_sz);
      break;
    }
    sem_wait(block_ledger->ledger_sem);
    if (block_ledger->num_blocks >= block_ledger->total_blocks) {
      printf("Bloco não pode ser escrito, limite de blocos atingido\n");
      printf("A terminar o programa\n");
      sem_post(block_ledger->ledger_sem);
      break;
    }
    sem_post(block_ledger->ledger_sem);
    Block *blk = (Block *)raw;

    int valid = verify_nonce(blk);
    tx = (Transaction *)((char *)blk + sizeof(Block));

    TransactionPoolEntry *transactions_ent_pool =
        (TransactionPoolEntry *)((char *)transactions_pool +
                                 sizeof(TransactionPool));

    for (unsigned int i = 0; i < config.transactions_per_block; i++) {
      for (unsigned int j = 0; j < transactions_pool->atual; j++) {
        if (strcmp(transactions_ent_pool[j].transaction.transaction_id,
                   tx[i].transaction_id) == 0) {
          if (transactions_ent_pool[j].occupied == 0) {
            valid = 0;
          }
          break;
        }
      }
    }

    printf("\n=== Recebido bloco %.*s (%s) ===\n", TX_ID_LEN, blk->block_id,
           valid ? "VÁLIDO" : "INVÁLIDO");
    if (valid) {

      for (unsigned int i = 0; i < config.transactions_per_block; i++) {
        for (unsigned int j = 0; j < transactions_pool->atual; j++) {
          if (strcmp(transactions_ent_pool[j].transaction.transaction_id,
                     tx[i].transaction_id) == 0) {
            transactions_ent_pool[j].occupied = 0;
            break;
          }
        }
      }
      for (unsigned int i = 0; i < config.transactions_per_block; i++) {
        printf("  - TX %2d: id=\"%s\" reward=%d\n", i, tx[i].transaction_id,
               tx[i].reward);
      }
      StatsMsg msg;
      msg.mtype = 1;
      msg.miner_id = atoi(strchr(blk->block_id, '-') + 1);
      msg.valid = valid;
      msg.credits = valid ? sum_rewards(tx, config.transactions_per_block) : 0;
      msg.n_tx = config.transactions_per_block;
      for (int i = 0; i < msg.n_tx; i++) {
        msg.tx_ts[i] = tx[i].timestamp;
      }
      msg.block_ts = blk->timestamp;

      // enviar
      if (msgsnd(stats_qid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd to stats");
      }
      sem_wait(block_ledger->ledger_sem);
      Block *dest =
          (Block *)((char *)block_ledger + sizeof(BlockchainLedger) +
                    block_ledger->num_blocks *
                        (sizeof(Block) +
                         sizeof(Transaction) * config.transactions_per_block));
      Transaction *dest_tx = (Transaction *)((char *)dest + sizeof(Block));
      dest->nonce = blk->nonce;
      dest->timestamp = blk->timestamp;
      memcpy(dest_tx, tx, sizeof(Transaction) * config.transactions_per_block);
      strcpy(dest->block_id, blk->block_id);
      strcpy(dest->previous_hash, blk->previous_hash);
      block_ledger->num_blocks++;
      memcpy(block_ledger->hash_atual, results->hash, HASH_SIZE);
      sem_post(block_ledger->ledger_sem);

      printf("Bloco escrito \n");
    }
  }

  close(fd);
  free(raw);
  free(results);
  sem_close(transactions_pool->transaction_pool_sem);

  sem_close(transactions_pool->tp_access_pool);

  sem_close(block_ledger->ledger_sem);

  sem_close(log_file_mutex);
  log_file_mutex = NULL;
  shmdt(transactions_pool);
  shmdt(block_ledger);
  shmctl(shmidledger, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);

  printf("Validator terminating cleanly.\n");
  return 0;
}

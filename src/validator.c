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

Transaction *tx;
int stats_qid;
volatile sig_atomic_t should_exit = 0;
int validator_id;
// sighandler that manages flag
void handle_term(int sig) {
  if (sig)
    should_exit = 1;
}

// sum rewards, used for the statistics
int sum_rewards(Transaction *tx, unsigned int num) {
  int sum = 0;
  for (unsigned int i = 0; i < num; i++)
    sum += tx[i].reward;
  return sum;
}

int validator() {
  struct sigaction sa = {.sa_handler = handle_term};
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  const size_t block_sz = get_transaction_block_size();
  unsigned char *raw = malloc(block_sz);
  PoWResult *results = malloc(sizeof(PoWResult));
  if (!raw || !results) {
    perror("malloc");
    return 1;
  }

  int fd_read = open("VALIDATOR_INPUT", O_RDONLY | O_NONBLOCK);
  if (fd_read < 0) {
    perror("open VALIDATOR_INPUT");
    return 1;
  }

  char valbuf[128];
  sprintf(valbuf, "Validator %d", validator_id);
  while (!should_exit) {
    ssize_t r = read(fd_read, raw, block_sz);
    ssize_t rr = read(fd_read, results, sizeof(*results));
    if (should_exit)
      break;

    if (r == 0 || rr == 0) {
      usleep(200000);
      continue;
    }
    if (r < 0 && (errno == EINTR || errno == EAGAIN)) {
      continue;
    }
    if (r < 0) {
      perror("read");
      break;
    }
    if ((size_t)r < block_sz) {
      fprintf(stderr, "Bloco incompleto: %zd/%zu bytes\n", r, block_sz);
      continue;
    }

    Block *blk = (Block *)raw;
    int valid = verify_nonce(blk);
    tx = (Transaction *)((char *)blk + sizeof(Block));

    sem_wait(block_ledger->ledger_sem);
    if (block_ledger->num_blocks >= block_ledger->total_blocks) {
      sem_post(block_ledger->ledger_sem);
      fprintf(stderr, "Ledger cheio, a sair\n");
      break;
    }
    sem_post(block_ledger->ledger_sem);

    sem_wait(transactions_pool->tp_access_pool);

    TransactionPoolEntry *ent =
        (TransactionPoolEntry *)((char *)transactions_pool +
                                 sizeof(TransactionPool));
    unsigned int limit = transactions_pool->atual;
    for (unsigned int i = 0; i < config.transactions_per_block; i++) {
      if (i == 0) {
        for (unsigned int j = 0; j < limit; j++) {
          ent[j].age++;
          if (ent[j].age > 50) {
            ent[j].age = 0;
            if (ent[j].transaction.reward < 3)
              ent[j].transaction.reward++;
          }
        }
      }
      // se tx não encontrada ou já vazia, invalida
      bool found = false;
      for (unsigned int j = 0; j < limit; j++) {
        if (strcmp(ent[j].transaction.transaction_id, tx[i].transaction_id) ==
            0) {
          found = true;
          if (!ent[j].occupied)
            valid = 0;
          break;
        }
      }
      if (!found)
        valid = 0;
    }

    if (valid) {
      unsigned int removed = 0;
      for (unsigned int i = 0; i < config.transactions_per_block; i++) {
        for (unsigned int j = 0; j < transactions_pool->atual; j++) {
          if (strcmp(ent[j].transaction.transaction_id, tx[i].transaction_id) ==
              0) {
            unsigned int tail = transactions_pool->atual - (j + 1);
            if (tail > 0) {
              memmove(&ent[j], &ent[j + 1], sizeof(*ent) * tail);
            }
            transactions_pool->atual--;
            removed++;
            break;
          }
        }
      }
    }

    sem_post(transactions_pool->tp_access_pool);

    char st[128];
    if (valid) {
      sprintf(st, "Bloco %.*s válido", TX_ID_LEN, blk->block_id);
      write_logfile(st, valbuf);
      printf("\n=== Recebido bloco %.*s: VÁLIDO ===\n", TX_ID_LEN,
             blk->block_id);
      sem_wait(block_ledger->ledger_sem);
      for (unsigned int transs = 0; transs < config.transactions_per_block;
           transs++) {
        printf("Transação %d: %s\n", transs, tx[transs].transaction_id);
      }
      BlockchainLedger *L = block_ledger;
      Block *dest =
          (Block *)((char *)L + sizeof(*L) +
                    L->num_blocks *
                        (sizeof(Block) +
                         sizeof(Transaction) * config.transactions_per_block));
      Transaction *dest_tx = (Transaction *)((char *)dest + sizeof(Block));

      dest->nonce = blk->nonce;
      dest->timestamp = blk->timestamp;
      memcpy(dest_tx, tx, sizeof(Transaction) * config.transactions_per_block);
      strcpy(dest->block_id, blk->block_id);
      strcpy(dest->previous_hash, blk->previous_hash);

      L->num_blocks++;
      memcpy(L->hash_atual, results->hash, HASH_SIZE);
      sem_post(block_ledger->ledger_sem);
      pthread_cond_signal(&block_ledger->cond_ct);
    } else {
      sprintf(st, "Bloco %.*s inválido", TX_ID_LEN, blk->block_id);
      sprintf(valbuf, "Validator %d", validator_id);
      write_logfile(st, valbuf);
      printf("\n=== Recebido bloco %.*s: INVÁLIDO ===\n", TX_ID_LEN,
             blk->block_id);
    }

    StatsMsg msg = {.mtype = 1};
    msg.miner_id = atoi(strchr(blk->block_id, '-') + 1);
    msg.valid = valid;
    msg.credits = valid ? sum_rewards(tx, config.transactions_per_block) : 0;
    msg.n_tx = config.transactions_per_block;
    for (unsigned int i = 0; i < msg.n_tx; i++)
      msg.tx_ts[i] = tx[i].timestamp;
    msg.block_ts = blk->timestamp;
    if (msgsnd(stats_qid, &msg, sizeof(msg) - sizeof(long), 0) == -1)
      perror("msgsnd to stats");
  }

  close(fd_read);
  free(raw);
  free(results);
  write_logfile("Validator terminado ", valbuf);
  printf("Validator terminando.\n");
  return 0;
}

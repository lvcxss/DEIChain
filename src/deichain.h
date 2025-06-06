#ifndef DEICHAIN_H
#define DEICHAIN_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define HASH_SIZE 65
#define MAX_TX 32
#define TX_ID_LEN 64
// data strutcutes usued in the project
typedef struct {
  unsigned int num_miners;
  unsigned int tx_pool_size;
  unsigned int transactions_per_block;
  unsigned int blockchain_blocks;
} Config;

typedef struct {
  char transaction_id[TX_ID_LEN];
  int reward;
  pid_t sender_id;
  int receiver_id;
  float value;
  time_t timestamp;
} Transaction;

typedef struct {
  int occupied;
  int age;
  Transaction transaction;
} TransactionPoolEntry;

typedef struct {
  unsigned int atual, max_size, transactions_block;
  sem_t *tp_access_pool;
  sem_t *sem_min_tx;
  // cv for validator controller
  pthread_mutex_t mt_vc;
  pthread_cond_t cond_vc;
} TransactionPool;

typedef struct {
  char block_id[TX_ID_LEN];
  time_t timestamp;
  char previous_hash[HASH_SIZE];
  unsigned int nonce;
} Block;

typedef struct {
  sem_t *ledger_sem;
  int total_blocks;
  int num_blocks;
  char hash_atual[HASH_SIZE];
  pthread_mutex_t mt_ct;
  pthread_cond_t cond_ct;
} BlockchainLedger;

typedef struct {
  long mtype;
  int miner_id;
  int valid;
  int credits;
  unsigned int n_tx;
  time_t tx_ts[MAX_TX];
  time_t block_ts;
} StatsMsg;

extern int *transactionid;
extern int *block_index;
extern int stats_qid;
extern Config config;
extern int validator_id;
extern sem_t *log_file_mutex;

extern TransactionPool *transactions_pool;
extern TransactionPoolEntry *transactions;
extern BlockchainLedger *block_ledger;

int write_logfile(char *message, char *typemsg);
int write_ledger_logfile();
void show_block(Block *block, Transaction *t);
static inline size_t get_transaction_block_size() {
  if (config.transactions_per_block == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(Block) + config.transactions_per_block * sizeof(Transaction);
}
#endif

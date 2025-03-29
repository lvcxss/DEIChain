#ifndef DEICHAIN_H
#define DEICHAIN_H

#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

void write_logfile(char *message, char *typemsg);
// data strutcutes usued in the project
typedef struct {
  unsigned int num_miners;
  unsigned int tx_pool_size;
  unsigned int transactions_per_block;
  unsigned int blockchain_blocks;
} Config;

typedef struct {
  int transaction_id;
  int reward;
  pid_t sender_id;
  int receiver_id;
  float value;
  time_t timestamp;
} Transaction;

typedef struct {
  int empty;
  int age;
  Transaction transaction;
} TransactionPoolEntry;

typedef struct {
  int current_block_id;
  TransactionPoolEntry transactions[];
} TransactionPool;
typedef struct {
  int block_id;
  time_t timestamp;
  int nonce;
  char previous_hash[65];
  Transaction transactions[];
} Block;

typedef struct {
  sem_t ledger_sem;
  int total_blocks;
  Block blocks[];
} BlockchainLedger;

extern int shmid;
extern int shmidtransactions;
extern int shmidledger;
extern int shmidblockindex;
extern int *transactionid;
extern int *transactions_pool_index;
extern int *block_index;
extern Config config;
extern pthread_mutex_t logfilemutex;
extern TransactionPool *transcations_pool;
extern BlockchainLedger *block_ledger;

#endif

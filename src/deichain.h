#ifndef DEICHAIN_H
#define DEICHAIN_H

#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#define HASH_SIZE 65
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
  int id;
  unsigned int atual, max_size;
  sem_t *transaction_pool_sem;
  sem_t *tp_access_pool;
  // cv for minimum transactions (miner should wait for this)
  pthread_mutex_t mt_min;
  pthread_cond_t cond_min;
} TransactionPool;

typedef struct {
  char block_id[TX_ID_LEN];
  time_t timestamp;
  char previous_hash[HASH_SIZE];
  unsigned int nonce;
} Block;

typedef struct {
  sem_t ledger_sem;
  int total_blocks;
  int num_blocks;
} BlockchainLedger;

extern int shmid;
extern int shmidtransactions;
extern int shmidledger;
extern int shmidblockindex;
extern int *transactionid;
extern int *block_index;
extern int miner_should_exit;
extern Config config;
extern pthread_mutex_t logfilemutex;
extern TransactionPool *transactions_pool;
extern TransactionPoolEntry *transactions;
extern BlockchainLedger *block_ledger;

int write_logfile(char *message, char *typemsg);
void showBlock(Block *block);
#endif

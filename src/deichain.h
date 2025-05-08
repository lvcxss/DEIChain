#ifndef DEICHAIN_H
#define DEICHAIN_H

#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

int write_logfile(char *message, char *typemsg);
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
  int occupied;
  int age;
  Transaction transaction;
} TransactionPoolEntry;

typedef struct {
  int id, max_size;
  sem_t *transaction_pool_sem;
} TransactionPool;

typedef struct {
  int block_id;
  time_t timestamp;
  int nonce;
  char previous_hash[65];
  char initial_hash[65]; //maybe not necessary but not sure
  Transaction transactions[];
} Block;

typedef struct{
  long int result; // using long int as an indicator of result
  int time_taken;
  long int miner_id;
  long int block_credit;
}ValidationMessage;

typedef struct {
  sem_t ledger_sem;
  int total_blocks;
  int num_blocks;
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
extern Transaction *transactions_pool; // já é o ponteiro para as transactions ?
extern BlockchainLedger *block_ledger; 

#endif

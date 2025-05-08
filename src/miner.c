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

int check_difficulty(const char *hash, const int reward) {
  int minimum = 4; // minimum difficult

  int zeros = 0;
  DifficultyLevel difficulty = getDifficultFromReward(reward);

  while (hash[zeros] == '0') {
    zeros++;
  }

  // At least `minimum` zeros must exist
  if (zeros < minimum)
    return 0;

  char next_char = hash[zeros];

  switch (difficulty) {
  case EASY: // 0000[0-b]
    if ((zeros == 4 && next_char <= 'b') || zeros > 4)
      return 1;
    break;
  case NORMAL: // 00000
    if (zeros >= 5)
      return 1;
    break;
  case HARD: // // 00000[0-b]
    if ((zeros == 5 && next_char <= 'b') || zeros > 5)
      return 1;
    break;
  default:
    fprintf(stderr, "Invalid Difficult\n");
    exit(2);
  }

  return 0;
}

unsigned char *serialize_block(const Block *block, size_t *sz_buf) {
  // We must subtract the size of the pointer, the static block does not have
  // the pointer
  *sz_buf = get_transaction_block_size() - sizeof(Transaction *);

  unsigned char *buffer = malloc(*sz_buf);
  if (!buffer)
    return NULL;

  memset(buffer, 0, *sz_buf);

  unsigned char *p = buffer;

  memcpy(p, block->block_id, TX_ID_LEN);
  p += TX_ID_LEN;

  memcpy(p, block->previous_hash, HASH_SIZE);
  p += HASH_SIZE;

  memcpy(p, &block->timestamp, sizeof(time_t));
  p += sizeof(time_t);

  Transaction *tx = (Transaction *)((char *)block + sizeof(Block));

  for (size_t i = 0; i < config.transactions_per_block; ++i) {
    memcpy(p, &tx[i], sizeof(Transaction));
    p += sizeof(Transaction);
  }

  memcpy(p, &block->nonce, sizeof(unsigned int));
  p += sizeof(unsigned int);

  return buffer;
}

void compute_sha256(const Block *block, char *output) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  // create a static buffer to copy data
  size_t buffer_sz;

  // since the Block has a pointer we must serialize the block to a buffer
  unsigned char *buffer = serialize_block(block, &buffer_sz);

  SHA256(buffer, buffer_sz, hash);
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(output + (i * 2), "%02x", hash[i]);
  }
  output[SHA256_DIGEST_LENGTH * 2] = '\0';
  free(buffer);
}

int get_max_transaction_reward(const Block *block, const int txs_per_block) {
  if (block == NULL)
    return 0;

  int max_reward = 0;
  TransactionPoolEntry *tpe = transactions;
  for (int i = 0; i < txs_per_block; ++i) {
    int reward = tpe[i].transaction.reward;
    if (reward > max_reward) {
      max_reward = reward;
    }
  }

  return max_reward;
}

/* Proof-of-Work function */
PoWResult proof_of_work(Block *block) {
  PoWResult result;

  result.elapsed_time = 0.0;
  result.operations = 0;
  result.error = 0;

  block->nonce = 0;

  int reward = get_max_transaction_reward(block, config.transactions_per_block);

  char hash[SHA256_DIGEST_LENGTH * 2 + 1];
  clock_t start = clock();

  while (1) {
    compute_sha256(block, hash);

    if (check_difficulty(hash, reward)) {
      result.elapsed_time = (double)(clock() - start) / CLOCKS_PER_SEC;
      strcpy(result.hash, hash);
      return result;
    }
    block->nonce++;
    if (block->nonce > POW_MAX_OPS) {
      fprintf(stderr, "Giving up\n");
      result.elapsed_time = (double)(clock() - start) / CLOCKS_PER_SEC;
      result.error = 1;
      return result;
    }
    if (DEBUG && block->nonce % 100000 == 0)
      printf("Nounce %d\n", block->nonce);
    result.operations++;
  }
}

DifficultyLevel getDifficultFromReward(const int reward) {
  if (reward <= 1)
    // 0000
    return EASY;

  if (reward == 2)
    // 00000
    return NORMAL;

  // For reward > 2
  // 00000[0-b]
  return HARD;
}

void *mine(void *idp) {
  while (!miner_should_exit) {

    printf("Miner %d waiting for transactions\n", *((int *)idp));
    pthread_mutex_lock(&transactions_pool->mt_min);

    while (transactions_pool->atual < config.transactions_per_block &&
           !miner_should_exit) {
      pthread_cond_wait(&transactions_pool->cond_min,
                        &transactions_pool->mt_min);
    }

    if (miner_should_exit) {
      pthread_mutex_unlock(&transactions_pool->mt_min);
      return NULL;
    }

    printf("Miner %d has enough transactions\n", *((int *)idp));
    pthread_mutex_unlock(&transactions_pool->mt_min);
    Block *new_block = malloc(get_transaction_block_size());
    if (!new_block) {
      printf("Erro ao alocar memoria");
    }
    unsigned int tid = (*((int *)idp));
    sem_wait(block_ledger->ledger_sem);
    snprintf(new_block->block_id, TX_ID_LEN, "BLOCK-%lu-%d", (unsigned long)tid,
             block_ledger->num_blocks);
    memcpy(new_block->previous_hash, block_ledger->hash_atual, HASH_SIZE);
    sem_post(block_ledger->ledger_sem);
    int s = (transactions_pool->atual > transactions_pool->max_size)
                ? transactions_pool->max_size
                : transactions_pool->atual;
    int numm = 0;
    Transaction *transactions_block =
        (Transaction *)((char *)new_block + sizeof(Block));
    printf("A selecionar transactions\n");
    for (int i = 0; i < s; i++) {
      if (transactions[i].occupied) {
        memcpy(&transactions_block[numm], &transactions[i].transaction,
               sizeof(Transaction));
        numm++;
      }
    }

    printf("Miner %d creating block\n", *((int *)idp));
    transactions_block = (Transaction *)((char *)new_block + sizeof(Block));
    int max_reward = 0;
    for (unsigned int i = 0; i < config.transactions_per_block; i++)
      if (transactions_block[i].reward > max_reward)
        max_reward = transactions_block[i].reward;
    size_t sz;
    unsigned char *buffer = serialize_block(new_block, &sz);
    printf("Bloco serializado\n");
    PoWResult result;
    do {
      printf("before proof\n");
      result = proof_of_work(new_block);
      printf("after proof\n");
      new_block->timestamp = time(NULL);
    } while (result.error == 1);
    printf("Enviar por pipe\n");
    int pipe_fd = open("VALIDATOR_INPUT", O_WRONLY);

    if (pipe_fd == -1) {
      perror("Erro ao abrir o pipe");
      return NULL;
    }
    write(pipe_fd, new_block, get_transaction_block_size());

    close(pipe_fd);
    free(buffer);
    free(new_block);
  }
  printf("Miner %d finished\n", *((int *)idp));
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
}

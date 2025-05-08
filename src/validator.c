// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include "pow.h"
#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
// verifying pow
// verifying hash
//  ageing transactions
Transaction *tx;

volatile sig_atomic_t should_exit = 0;

void handle_term(int sig) {
  if (sig) {
    should_exit = 1;
  }
}

int verify_nonce(const Block *block) {
  char hash[SHA256_DIGEST_LENGTH * 2 + 1];
  int reward = get_max_transaction_reward(block, config.transactions_per_block);
  compute_sha256(block, hash);
  return check_difficulty(hash, reward);
}

int validator() {
  struct sigaction sa;
  sa.sa_handler = handle_term;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  key_t key_ledger = ftok("config.cfg", 66);
  int shmid_ledger = shmget(key_ledger, 0, 0666);
  block_ledger = shmat(shmid_ledger, NULL, 0);

  const size_t block_sz = get_transaction_block_size();
  unsigned char *raw = malloc(block_sz);
  if (!raw) {
    perror("malloc");
    return 1;
  }

  int fd;
  if ((fd = open("VALIDATOR_INPUT", O_RDONLY)) < 0) {
    perror("open VALIDATOR_INPUT");
    free(raw);
    return 1;
  }

  while (!should_exit) {
    ssize_t r = read(fd, raw, block_sz);
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

    Block *blk = (Block *)raw;

    int valid = verify_nonce(blk);
    printf("\n=== Recebido bloco %.*s (%s) ===\n", TX_ID_LEN, blk->block_id,
           valid ? "VÁLIDO ✅" : "INVÁLIDO ❌");
    tx = (Transaction *)((char *)blk + sizeof(Block));
    for (unsigned int i = 0; i < config.transactions_per_block; i++) {
      printf("  - TX %2d: id=\"%s\" reward=%d\n", i, tx[i].transaction_id,
             tx[i].reward);
    }
  }

  close(fd);
  free(raw);
  printf("Validator terminating cleanly.\n");
  return 0;
}

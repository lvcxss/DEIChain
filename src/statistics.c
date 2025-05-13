//
#include "statistics.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
volatile sig_atomic_t usr1_received = 0;
volatile sig_atomic_t stats_should_exit = 0;

MinerStats *miners_stats = NULL;
int total_valid_blocks = 0;
int total_invalid_blocks = 0;

void handle_sigusr1(int sig) {
  if (sig == SIGUSR1) {
    usr1_received = 1;
  }
}
void handle_sigtermstats(int signum) {
  if (signum == SIGTERM) {
    stats_should_exit = 1;
  }
}

void print_current_stats() {
  for (unsigned int i = 0; i < config.num_miners; i++) {
    printf("Minerador %d:\n", i + 1);
    printf("  Blocos válidos: %d\n", miners_stats[i].valid_blocks);
    printf("  Blocos inválidos: %d\n", miners_stats[i].invalid_blocks);
    printf("  Créditos acumulados: %d\n", miners_stats[i].total_credits);
    if (miners_stats[i].total_transactions > 0) {
      printf("  Tempo médio por transação: %.2f ms\n",
             (miners_stats[i].total_processing_time /
              miners_stats[i].total_transactions) *
                 1000);
    }
  }
  printf("Total de blocos na blockchain: %d\n", total_valid_blocks);
  printf("Total de blocos rejeitados: %d\n", total_invalid_blocks);
}

void print_statistics() {
  struct sigaction sa;
  sa.sa_handler = handle_sigusr1;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
  struct sigaction sa2;
  sa2.sa_handler = handle_sigtermstats;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_flags = 0;
  sigaction(SIGTERM, &sa2, NULL);

  miners_stats = malloc(config.num_miners * sizeof(MinerStats));
  memset(miners_stats, 0, config.num_miners * sizeof(MinerStats));

  key_t key = ftok("config.cfg", 67);
  int stats_qid = msgget(key, 0666);
  if (stats_qid == -1) {
    perror("msgget em Statistics");
    exit(1);
  }

  StatsMsg msg;
  while (!stats_should_exit) {
    if (usr1_received) {
      printf("========== SIGURS1 RECEBIDO ===========\n");

      print_current_stats();
      usr1_received = 0;
    }

    if (msgrcv(stats_qid, &msg, sizeof(msg) - sizeof(long), 1, IPC_NOWAIT) ==
        -1) {
      if (errno == ENOMSG) {
        continue;
      }
      perror("msgrcv");
      break;
    }

    int miner_id = msg.miner_id - 1;
    if (miner_id < 0 || miner_id >= (int)config.num_miners) {
      fprintf(stderr, "ID de minerador inválido: %d\n", miner_id);
      continue;
    }

    if (msg.valid) {
      miners_stats[miner_id].valid_blocks++;
      total_valid_blocks++;
      miners_stats[miner_id].total_credits += msg.credits;
    } else {
      miners_stats[miner_id].invalid_blocks++;
      total_invalid_blocks++;
    }

    for (unsigned int i = 0; i < msg.n_tx; i++) {
      double processing_time = difftime(msg.block_ts, msg.tx_ts[i]);
      miners_stats[miner_id].total_processing_time += processing_time;
      miners_stats[miner_id].total_transactions++;
    }
  }

  printf("\n=== Estatísticas Finais ===\n");
  print_current_stats();
  free(miners_stats);
  sem_close(transactions_pool->transaction_pool_sem);

  sem_close(transactions_pool->tp_access_pool);

  sem_close(block_ledger->ledger_sem);

  sem_close(log_file_mutex);
  log_file_mutex = NULL;
  exit(0);
}

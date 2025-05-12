// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva

#include "deichain.h"
#include "transaction.h"
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
sem_t *sem;
void signal_handler(int signum) {
  if (signum == SIGINT) {
    char msg[256];
    printf("Killing transaction generator\n");
    sprintf(msg, "SIGINT received on transaction generator with id: %d.",
            getpid());
    sem_close(sem);

    exit(0);
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  if (argc != 3) {
    printf("Usage: %s <reward> <sleep time>\n", argv[0]);
    return 1;
  }
  sem = sem_open("/transaction_pool_sem", O_CREAT, 0666, 1);
  key_t key = ftok("config.cfg", 65);
  int shmid = shmget(key, 0, 0666);
  TransactionPool *transaction_pool = shmat(shmid, NULL, 0);
  TransactionPoolEntry *transactions =
      (TransactionPoolEntry *)((char *)transaction_pool +
                               sizeof(TransactionPool));

  int reward = atoi(argv[1]);
  if (reward == 0 && argv[1][0] != '0') {
    printf("Reward must be a number\n");
    exit(1);
  }
  int sleepTime = atoi(argv[2]);
  if (sleepTime == 0 && argv[2][0] != '0') {
    printf("Sleep time must be a number\n");
    exit(1);
  }

  if (reward > 3 || reward < 1) {
    printf("Reward must be between 1 and 3\n");
    exit(1);
  }

  if (sleepTime > 3000 || sleepTime < 200) {
    printf("Sleep time must be between 200 and 3000\n");
    exit(1);
  }
  char msg[256];
  Transaction t;
  unsigned int id;
  int done = 0;
  while (!done) {
    t = create_transaction(reward, 4);

    sprintf(msg, "Transaction with reward %d and sleep time %d pid: %d created",
            t.reward, sleepTime, getpid());
    TransactionPoolEntry ent;
    ent.transaction = t;
    ent.age = 0;
    ent.occupied = 1;
    printf("%s\n", msg);
    sem_wait(sem);

    id = transaction_pool->atual;
    if (id == transaction_pool->max_size) {
      sem_post(sem);
      done = 1;
    }
    transactions[id] = ent;
    printf("%d", transaction_pool->atual);
    transaction_pool->atual++;
    transaction_pool->available++;
    if (transaction_pool->atual > transaction_pool->transactions_block) {
      pthread_cond_broadcast(&transaction_pool->cond_min);
    }
    sem_post(sem);
    sleep(sleepTime / 1000);
  }
  sem_close(sem);
  return 1;
}

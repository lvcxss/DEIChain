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
sem_t *sem, *sem_min_tx;
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
  sem = sem_open("/tp_access_pool", O_CREAT, 0666, 1);
  if (sem == SEM_FAILED) {
    printf("Para o tx generator funcionar, deverá ter executado o main "
           "previamente\n");
    perror("sem_open");
    exit(1);
  }
  sem_min_tx = sem_open("/sem_min_tx", 0);
  if (sem_min_tx == SEM_FAILED) {
    perror("sem_open /sem_min_tx");
    exit(1);
  }

  key_t key = ftok("config.cfg", 65);
  int shmid = shmget(key, 0, 0666);
  if (shmid == -1) {
    printf("Para o tx generator funcionar, deverá ter executado o main "
           "previamente\n");
    exit(1);
  }
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
  srand(time(NULL));
  int done = 0;
  unsigned int prev = transaction_pool->max_size;
  // this cycle does:
  // checks for an empty block and places a transaction in it
  // if not places it in the end
  // lets miners work if the max index of the array is greater then the number
  // of transactions in a block
  //
  while (!done) {
    if (transaction_pool->max_size != prev) {
      printf("O processo principal terminou, a matar txgen\n");
      return 0;
    }
    t = create_transaction(reward, rand() / 1000);
    int found = 0;
    sprintf(msg, "Transaction with reward %d and sleep time %d pid: %d created",
            t.reward, sleepTime, getpid());
    TransactionPoolEntry ent;
    ent.transaction = t;
    ent.age = 0;
    ent.occupied = 1;
    printf("%s\n", msg);
    sem_wait(sem);

    for (unsigned int ii = 0; ii < transaction_pool->atual && found == 0;
         ii++) {
      if (transactions[ii].occupied == 0) {
        transactions[ii] = ent;
        found = 1;
      }
    }
    if (!found) {
      transactions[transaction_pool->atual] = ent;
      transaction_pool->atual++;
      if (transaction_pool->atual >= transaction_pool->max_size) {
        printf("A transactions pool está cheia, a encerrar este generador\n");
        done = 1;
      }
    }

    if (transaction_pool->atual > transaction_pool->transactions_block) {
      sem_post(sem_min_tx);
    }
    pthread_cond_signal(&transaction_pool->cond_vc);

    sem_post(sem);
    usleep(sleepTime * 1000);
  }
  sem_close(sem);
  return 1;
}

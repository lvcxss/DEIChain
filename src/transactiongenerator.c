#include "deichain.h"
#include "transaction.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
int *transactionid;
Transaction create_transaction(int reward, int value) {
  Transaction t = {.reward = reward,
                   .sender_id = getpid(),
                   .receiver_id = rand() % 1000,
                   .value = value,
                   .transaction_id = getpid() + *transactionid,
                   .timestamp = time(NULL)};
  // como o transaction_id é a soma do pid do processo e do transactionid
  // a leitura e incrementacao da vairavel nao e algo critico, dai nao termos
  // usado semaforos extra
  (*transactionid)++;
  char msg[256];
  sprintf(msg, "Transaction with reward : %d and value : %d created", reward,
          value);
  // write_logfile(msg, "INFO");
  return t;
}

void print_transaction(Transaction t) {
  printf("Transaction ID : %d\n", t.transaction_id);
  printf("Sender ID : %d\n", t.sender_id);
  printf("Receiver ID : %d\n", t.receiver_id);
  printf("Value : %f\n", t.value);
  printf("Reward : %d\n", t.reward);
  printf("Timestamp : %ld\n", t.timestamp);
}

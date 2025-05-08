// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva

#include "deichain.h"
#include "transaction.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
int a = 0;
int *transactionid = &a;

Transaction create_transaction(int reward, int value) {
  Transaction t = {.reward = reward,
                   .sender_id = getpid(),
                   .receiver_id = rand() % 1000,
                   .value = value,
                   .timestamp = time(NULL)};
  snprintf(t.transaction_id, TX_ID_LEN, "TX-%d-%d", getpid(), a++);

  // como o transaction_id é a soma do pid do processo e do transactionid
  // a leitura e incrementacao da vairavel nao e algo critico, dai nao termos
  // usado semaforos extra
  char msg[256];
  sprintf(msg, "Transaction with reward : %d and value : %d created", reward,
          value);
  // write_logfile(msg, "INFO");
  return t;
}

// simple printing function for testing purposes (for now)
void print_transaction(TransactionPoolEntry t) {
  printf("Transaction ID : %s\n", t.transaction.transaction_id);
  printf("Sender ID : %d\n", t.transaction.sender_id);
  printf("Receiver ID : %d\n", t.transaction.receiver_id);
  printf("Value : %f\n", t.transaction.value);
  printf("Reward : %d\n", t.transaction.reward);
  printf("Timestamp : %ld\n", t.transaction.timestamp);
}

#include "controller.h"
#include <pthread.h>
#include <semaphore.h>
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
  return t;
}

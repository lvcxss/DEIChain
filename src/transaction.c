#include "transaction.h"
#include "controller.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

transaction *create_transaction(int val, int sender, int receiver, int value) {
  transaction *t;
  t->val = val;
  t->sender = sender;
  t->receiver = receiver;
  t->value = value;
  t->timestamp = time(NULL);

  pthread_mutex_lock(&transactionidmutex);
  printf("id atual: %d", *transactionid);
  t->id = *transactionid;
  *transactionid = *transactionid + 1;
  pthread_mutex_unlock(&transactionidmutex);
  return t;
}

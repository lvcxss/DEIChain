#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <time.h>

typedef struct transaction {
  int id;
  int val;
  int sender;
  int receiver;
  int value;
  time_t timestamp;

} Transaction;

#endif

#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <time.h>
typedef struct {
  int id;
  int val;
  int sender;
  int receiver;
  int value;
  time_t timestamp;

} transaction;

#endif

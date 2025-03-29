#ifndef TRANSACTION_H
#define TRANSACTION_H
#include "controller.h"
#include <time.h>
Transaction create_transaction(int reward, int value);
void print_transaction(Transaction transaction);
#endif

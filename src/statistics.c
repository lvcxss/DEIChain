#include "statistics.h"
#include <stdio.h>

void printStatistics(unsigned int transactionsCreated, unsigned int numMiners) {
  printf("transactions created: %d\n", transactionsCreated);
  printf("number of miners: %d\n", numMiners);
}

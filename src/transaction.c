#include "transaction.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <reward> <sleep time>\n", argv[0]);
    return 1;
  }

  int reward = atoi(argv[1]);
  int sleepTime = atoi(argv[2]);

  if (reward > 3 || reward < 1) {
    printf("Reward must be between 1 and 3\n");
    exit(1);
  }

  if (sleepTime > 3000 || sleepTime < 200) {
    printf("Sleep time must be between 200 and 3000\n");
    exit(1);
  }

  printf("Transaction Created\n");

  return 0;
}
